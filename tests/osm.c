#include "forest.h"
#include "parser.h"
#include "helpers.h"

#include <assert.h>
#include <time.h>

#define DC_VAL 3        // terminal value for Don't care
#define NUM_OUT 5       // number of outcome values
#define BUF_SIZE 1024   // minterms buffer size

/*******************************************************************************************************
 *  
 *  The forest will be build in multi-roots, starting from long edge to terminal DC_VAL.
 *  Evaluated minterms with outcome vale i would be unioned into root i (if one input file) or root i+x 
 *  (if more than one file, x depends on the position in the input order), with terminal value 1.
 * 
 *  Before concretizing, ordering the roots in each group, reading minterms agan but union them into 
 *  roots after its corresponding root in the group, with terminal value 0.
 * 
 */



/*=======================================================================================================
 *                                           helper functions here
 =======================================================================================================*/
char*** init_minterms(int num_out, unsigned buf_size, unsigned num_bits)
{
    //
    char*** minterms;
    int x;
    unsigned int y, z;
    minterms = (char***)malloc(num_out * sizeof(char**));
    for (x = 0; x<num_out; x++) {
        minterms[x] = (char**)malloc(buf_size * sizeof(char*));
        for (y = 0; y<buf_size; y++) {
            minterms[x][y] = (char*) malloc((num_bits+2)*sizeof(char));
        }
    }

    for (x = 0; x<num_out; x++) {
        for (y = 0; y<buf_size; y++) {
            for (z = 0; z<num_bits+2; z++) {
                minterms[x][y][z] = 0;
            }
        }
    }
    return minterms;
}

void free_minterms(char*** minterms, int num_out, unsigned buf_size, unsigned num_bits)
{
    //
    if (!minterms) {
        printf("null minterms to free!\n");
        exit(1);
    }
    int x;
    unsigned int y, z;
    for (x = 0; x<num_out; x++) {
        for (y = 0; y<buf_size; y++) {
            for (z = 0; z<num_bits+2; z++) {
                minterms[x][y][z] = 0;
            }
            free(minterms[x][y]);
        }
        free(minterms[x]);
    }
    free(minterms);
}

void read_removeList(const char* infile, int* removes, int N)
{
    //
    FILE* inf = fopen(infile, "r");
    if (!inf) {
        printf("Failed to open the file.\n");
        exit(1);
    }
    if (!removes) {
        printf("Failed to allocate removes list.\n");
        exit(1);
    }

    int count = 0;
    while (count < N && fscanf(inf, "%d", &removes[count]) == 1) {
        count++;
    }

    fclose(inf);
}

uint64_t count_nodes(rexdd_forest_t* F, rexdd_edge_t* root, int dc, int num1, int num2)
{
    unmark_forest(F);
    for (int i=num1; i<num2; i++) {
        if (i != dc) {
            mark_nodes(F, root[i].target);
        }
    }
    uint_fast64_t q, n;
    uint64_t num_nodes = 0;
    for (q=0; q<F->M->pages_size; q++) {
        const rexdd_nodepage_t *page = F->M->pages+q;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                num_nodes++;
            }
        } // for n
    } // for p
    return num_nodes;
}

rexdd_edge_t union_minterms(rexdd_forest_t* F, uint32_t K, rexdd_edge_t* root, char** minterms, int outcome, unsigned int N)
{
    //
    rexdd_edge_t ans;
    if (N == 0) {
        rexdd_set_edge(&ans,
                        root->label.rule,
                        root->label.complemented,
                        root->label.swapped,
                        root->target);
        return ans;
    }

    if (K == 0) {
        rexdd_node_handle_t termi;
        if ((outcome == 1) || (outcome == 0)) {
            termi = (root->label.complemented) ? rexdd_make_terminal(1-outcome) : rexdd_make_terminal(outcome);
        } else {
            termi = rexdd_make_terminal(outcome);
        }
        rexdd_set_edge(&ans,
                        rexdd_rule_X,
                        0,
                        0,
                        termi);
        return ans;
    }

    // Two-finger algorithm to sort 0,1 values in position K
    unsigned left = 0;
    unsigned right = N-1;

    for (;;) {
        // move left to first 1 value
        for ( ; left < right; left++) {
        if ('1' == minterms[left][K]) break;
        }
        // move right to first 0 value
        for ( ; left < right; right--) {
        if ('0' == minterms[right][K]) break;
        }
        // Stop?
        if (left >= right) break;
        // we have a 1 before a 0, swap them
        char* tmp = minterms[left];
        minterms[left] = minterms[right];
        minterms[right] = tmp;
        // For sure we can move them one spot
        ++left;
        --right;
    } // end loop

    if (left < N) {
        if ('0' == minterms[left][K]) left++;
        if (left < N) assert('1' == minterms[left][K]);
        if (left > 0) assert('0' == minterms[left-1][K]);
    }

    uint32_t root_skip;     // if the root edge is a long edge or not
    if (rexdd_is_terminal(root->target)) {
        root_skip = K;
    } else {
        root_skip = K - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, root->target));
    }
    rexdd_edge_t root_down0, root_down1;
    if (root_skip == 0) {
        // root edge does not skip ndoes at this level
        rexdd_edge_label_t l;
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge((root->label.swapped) ? &root_down1 : &root_down0,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root->target)));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge((root->label.swapped) ? &root_down0 : &root_down1,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root->target)));
        if (root->label.complemented) {
            rexdd_edge_com(&root_down0);
            rexdd_edge_com(&root_down1);
        }
    } else {
        // here means the root edge is a long edge (X, EL, EH, AL, AH)
        if (root->label.rule == rexdd_rule_X) {
            // this is easy
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EL(root->label.rule)) {
            // root edge is EL
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            0,
                            0,
                            rexdd_make_terminal(rexdd_is_one(root->label.rule)));
            rexdd_set_edge(&root_down1,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EH(root->label.rule)) {
            // root edge is EH
            rexdd_set_edge(&root_down0,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            0,
                            0,
                            rexdd_make_terminal(rexdd_is_one(root->label.rule)));
        } else if (rexdd_is_AL(root->label.rule)) {
            // root edge is AL
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AL edge skips >= 2 nodes
                root_down0.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EL1 : rexdd_rule_EL0;
            }
        } else if (rexdd_is_AH(root->label.rule)) {
            // root edge is AH
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AH edge skips >= 2 nodes
                root_down1.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EH1 : rexdd_rule_EH0;
            }
        }
    }

    // Build new node
    rexdd_unpacked_node_t tmp;
    tmp.level = K;
    tmp.edge[0] = union_minterms(F, K-1, &root_down0, minterms, outcome, left);
    tmp.edge[1] = union_minterms(F, K-1, &root_down1, minterms + left, outcome, N - left);

    ans.label.rule = rexdd_rule_X;
    ans.label.complemented = 0;
    ans.label.swapped = 0;

    rexdd_reduce_edge(F, K, ans.label, tmp, &ans);
    return ans;
}

void add(unsigned* minterms_state, char*** minterms, unsigned num_bits, const char* linebuf, int term)
{
    //  minterms_state[i] is the number of minterms for terminal i stored in minterms
    if (!minterms) {
        printf("NUll minterms\n");
        exit(1);
    }
    // int num_out;
    // for (num_out = 0; num_out<NUM_OUT; num_out++) {
    //     if (minterms_state[num_out]>BUF_SIZE) {
    //         printf("Too many minterms stored!\n");
    //         exit(1);
    //     }
    // }
    if (minterms_state[term]>=BUF_SIZE) {
        printf("Too many minterms stored! (%d / %u)\n",term, minterms_state[term]);
        exit(1);
    }
    unsigned i;
    for (i = 0; i<num_bits+2; i++) {
        if (!minterms[term]) {
            printf("null minterms[term]\n");
            exit(1);
        }
        if (!minterms_state) {
            printf("null minterms_state[term]\n");
            exit(1);
        }
        if (!minterms[term][minterms_state[term]]) {
            printf("null minterms[term][minterms_state[term]]\n");
            exit(1);
        }
        minterms[term][minterms_state[term]][i] = linebuf[i];
    }
    minterms_state[term]++;
}

/*-----------------------------------------------Restriction--------------------------------------------*/
unsigned char compare_osm(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2)
{
    // Base cases:
    if (rexdd_edges_are_equal(e1, e2)) return '=';
    if (rexdd_is_terminal(e1->target) && rexdd_terminal_value(e1->target) == DC_VAL) return '>';
    if (rexdd_is_terminal(e2->target) && rexdd_terminal_value(e2->target) == DC_VAL) return '<';
    
    // they may be both terminal but not DC value, it's !
    if (rexdd_is_terminal(e1->target) && rexdd_is_terminal(e2->target)) return '!';

    // ordering it for comparing
    bool swap = false;
    if (e1->target > e2->target) {
        rexdd_edge_t* t;
        t = e1;
        e1 = e2;
        e2 = t;
        swap = true;
    }

    unsigned char ans;

    // computing table? TBD

    uint_fast32_t lvl1, lvl2;
    if (rexdd_is_terminal(e1->target)) {
        lvl1 = 0;
    } else {
        lvl1 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e1->target));
    }
    if (rexdd_is_terminal(e2->target)) {
        lvl2 = 0;
    } else {
        lvl2 = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e2->target));
    }

    unsigned char cmp0, cmp1;
    rexdd_edge_t le1, le2, he1, he2;
    if (lvl1 < lvl2) {
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, e2->target), &le2.label);
        le2.target = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, e2->target));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, e2->target), &he2.label);
        he2.target = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, e2->target));
        if (e2->label.complemented) {
            rexdd_edge_com(&le2);
            rexdd_edge_com(&he2);
        }
        cmp0 = compare_osm(F, e1, (e2->label.swapped)? &he2 : &le2);
        cmp1 = compare_osm(F, e1, (e2->label.swapped)? &le2 : &he2);
    } else if (lvl1 > lvl2) {
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, e1->target), &le1.label);
        le1.target = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, e1->target));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, e1->target), &he1.label);
        he1.target = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, e1->target));
        if (e1->label.complemented) {
            rexdd_edge_com(&le1);
            rexdd_edge_com(&he1);
        }
        cmp0 = compare_osm(F, (e1->label.swapped)? &he1 : &le1, e2);
        cmp1 = compare_osm(F, (e1->label.swapped)? &le1 : &he1, e2);
    } else {
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, e2->target), &le2.label);
        le2.target = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, e2->target));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, e2->target), &he2.label);
        he2.target = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, e2->target));
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, e1->target), &le1.label);
        le1.target = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, e1->target));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, e1->target), &he1.label);
        he1.target = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, e1->target));
        if (e2->label.complemented) {
            rexdd_edge_com(&le2);
            rexdd_edge_com(&he2);
        }
        if (e1->label.complemented) {
            rexdd_edge_com(&le1);
            rexdd_edge_com(&he1);
        }

        cmp0 = compare_osm(F, (e1->label.swapped)? &he1 : &le1, (e2->label.swapped)? &he2 : &le2);
        cmp1 = compare_osm(F, (e1->label.swapped)? &le1 : &he1, (e2->label.swapped)? &le2 : &he2);
    }

    if ((cmp0 == '=') || (cmp0 == cmp1)) {
        ans = cmp1;
    } else {
        ans = '!';
    }

    if (ans == '<') return swap ? '>' : '<';
    if (ans == '>') return swap ? '<' : '>';
    return ans;
}

rexdd_edge_t osm(rexdd_forest_t* F, rexdd_edge_t* root, uint_fast32_t n)
{
    rexdd_edge_t ans;
    // terminal case
    if (rexdd_is_terminal(root->target)) {
        if (rexdd_terminal_value(root->target)!=DC_VAL) {
            rexdd_set_edge(&ans,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            return ans;
        }
    }
    // computing table TBD

    rexdd_edge_t lc, hc;
    rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M,root->target), &(lc.label));
    lc.target = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M,root->target));
    rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M,root->target), &(hc.label));
    hc.target = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M,root->target));
    uint_fast32_t levelc = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M,root->target));
    // printf("got child edges\n");
    if (root->label.complemented) {
        rexdd_edge_com(&lc);
        rexdd_edge_com(&hc);
    }
    
    // printf("lc's target is %llu\n", lc.target);
    // printf("hc's target is %llu\n", hc.target);

    unsigned char child_cmp;
    child_cmp = compare_osm(F, &lc, &hc);
    // printf("got comparing result\n");
    if (child_cmp == '>') {
        return osm(F, &hc, levelc-1);
    }
    if (child_cmp == '<') {
        return osm(F, &lc, levelc-1);
    }
    assert(child_cmp != '=');

    // new node

    rexdd_unpacked_node_t tmp;
    tmp.level = levelc;
    tmp.edge[0] = osm(F, &lc, tmp.level-1);
    tmp.edge[1] = osm(F, &hc, tmp.level-1);
    
    /*
     *  how to deal with complement bit? TBD
     */
    rexdd_edge_label_t l;
    l.rule = root->label.rule;
    l.complemented = 0;
    l.swapped = root->label.swapped;

    rexdd_reduce_edge(F, n, l, tmp, &ans);
    
    return ans;
}

/*=======================================================================================================
 *                                           main function here
 =======================================================================================================*/
int main(int argc, const char* const* argv)
{
    if (argc == 1) {
        printf("Need one input file\n");
        return 0;
    }

    FILE* fp;

    rexdd_forest_t F;
    rexdd_forest_settings_t s;
    rexdd_edge_t root_edge[5*(argc - 1)];
    char root_flag[5*(argc - 1)];           // mark the roots in use
    int dc_val = DC_VAL;
    // Initializing the root edges and don't care edge
    for (int i=0; i<5*(argc-1); i++) {
        rexdd_set_edge(&root_edge[i], rexdd_rule_X, 0, 0, rexdd_make_terminal(dc_val));
        root_flag[i] = 0;
    }

    const char* infile = 0;

    char term;
    unsigned num_inputbits;
    unsigned long num_f;
    int index = 0;
    unsigned long count_gc = 0;
    /*  Time    */
    clock_t start_time, end_time;
    start_time = clock();
    for (int n=1; n<argc; n++) {
        infile = argv[n];
        /*
         *  Initializing the parser
         */
        char fmt = 0 , comp = 0;
        char type[2];
        file_type(infile, type);
        fmt = type[0];
        comp = type[1];
        parser p;
        file_reader fr;
        init_file_reader(&fr, infile, comp);
        init_parser(&p, &fr);
        switch (fmt) {
            case 'p':
            case 'P':
                read_header_pla(&p);
                break;        
            case 'b':
            case 'B':
                read_header_bin(&p);
                break;
            default:
                printf("No parser for format %c\n",fmt);
        }
        printf("\tBuilding forest %d for %s\n", n, infile);
        printf("\tThe format is %c\n", fmt);
        printf("\tThe compress is %c\n", comp);
        printf("\tThe number of inbits is %u\n", p.inbits);
        printf("\tThe number of outbits is %u\n", p.outbits);
        printf("\tThe number of minterms is %lu\n", p.numf);

        num_inputbits = p.inbits;
        num_f = p.numf;
        char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level

        /*
         *  Initializing the forest
         */
        if (n == 1) {
            rexdd_default_forest_settings(p.inbits, &s);
            rexdd_init_forest(&F, &s);
        }

        /*
         *  Initializing the 3D minterms
         */
        char*** minterms;
        minterms =  init_minterms(NUM_OUT, BUF_SIZE, num_inputbits);
        unsigned minterms_state[NUM_OUT];
        int out_index;
        for (out_index=0; out_index<NUM_OUT; out_index++) {
            minterms_state[out_index] = 0;
        }

        /*
         *  Reading minterms and building forest
         */
        index = 0;
        count_gc = 0;
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p, inputbits, &term)) break;
            }
            
            index = term - '1' + 5*(n-1);
            if (root_flag[index] == 0) root_flag[index] = 1;

            add(minterms_state, minterms, num_inputbits, inputbits, term - '1');

            // check if reach the buffer size, and union minterms buffer
            for (out_index = 0; out_index<NUM_OUT; out_index++) {
                if (minterms_state[out_index] >= BUF_SIZE) {
                    root_edge[out_index+5*(n-1)] = union_minterms(&F, num_inputbits, &root_edge[out_index+5*(n-1)], minterms[out_index], 1, minterms_state[out_index]);
                    minterms_state[out_index] = 0;
                }
            }

            count_gc++;
            if (count_gc % (num_f / 5) == 0) {
                unmark_forest(&F);
                for (int i=0; i<5*(argc-1); i++) {
                    mark_nodes(&F, root_edge[i].target);
                }
                gc_unmarked(&F);
                printf("GC %d done\n", (int)(count_gc / (num_f / 5)));
            }
        }
        // flush remaining minterms
        for (out_index = 0; out_index<NUM_OUT; out_index++) {
            if (minterms_state[out_index] > 0) {
                root_edge[out_index+5*(n-1)] = union_minterms(&F, num_inputbits, &root_edge[out_index+5*(n-1)], minterms[out_index], 1, minterms_state[out_index]);
                minterms_state[out_index] = 0;
            }
        }

        free_parser(&p); // file reader will be free
        free_minterms(minterms, NUM_OUT, BUF_SIZE, num_inputbits);
        fp = fopen("concretizing_process.txt", "w+");
        fprintf(fp, "Base building: %d / %d\n", n, argc-1);
        fclose(fp);
    } // end of files for loop
    end_time = clock();
    printf("** reading and building time: %0.2f seconds **\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);
    printf("============================================================================\n");

/*
 *  At this point, forest is built!
 */

/*----------------------------------------Mark and count ALL root---------------------------------------*/
    uint_fast64_t q, n;
    uint64_t num_nodes = 0;
    for (int i=1; i<argc; i++) {
        unmark_forest(&F);
        for (int j=0+5*(i-1); j<5+5*(i-1); j++) {
            if (root_flag[j]==1) mark_nodes(&F, root_edge[j].target);
        }
        num_nodes = 0;
        for (q=0; q<F.M->pages_size; q++) {
            const rexdd_nodepage_t *page = F.M->pages+q;
            for (n=0; n<page->first_unalloc; n++) {
                if (rexdd_is_packed_marked(page->chunk+n)) {
                    num_nodes++;
                }
            } // for n
        } // for p
        printf("number of nodes (%s) in forest %d before removing and concretizing is %llu\n", TYPE, i, num_nodes);
    }
    // unmarking forest
    unmark_forest(&F);
    // marking nodes in use
    for (int i=0; i<5*(argc-1); i++) {
        if (rexdd_is_terminal(root_edge[i].target)) {
            printf("\tforest %d : %d is T%d\n",(i/5)+1, i%5, (int)rexdd_terminal_value(root_edge[i].target));
        } else {
            printf("\tforest %d : %d is %llu\n",(i/5)+1, i%5, root_edge[i].target);
        }
        mark_nodes(&F, root_edge[i].target);
    }
    // counting the total number
    for (q=0; q<F.M->pages_size; q++) {
        const rexdd_nodepage_t *page = F.M->pages+q;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                num_nodes++;
            }
        } // for n
    } // for p
    if (argc == 2) {
        printf("Total number of nodes (%s) in %s is %llu\n", TYPE, infile, num_nodes);
    } else {
        printf("Total number of nodes (%s) in forest is %llu\n", TYPE, num_nodes);
    }

/*----------------------------------------Mark and count ALL root---------------------------------------*/
    /*  w/o one root edge and count */
    uint64_t min_num_nodes = 0;
    int wo = 0;
    for (int j=1; j<argc; j++) {
        infile = argv[j];
        num_nodes = count_nodes(&F, root_edge, 0+5*(j-1), 0+5*(j-1), 5+5*(j-1));
        wo = 0+5*(j-1);
        for (int i=1+5*(j-1); i<5+5*(j-1); i++) {
            min_num_nodes = count_nodes(&F, root_edge, i, 0+5*(j-1), 5+5*(j-1));
            if (num_nodes > min_num_nodes) {
                num_nodes = min_num_nodes;
                wo = i;
            }
        }
        root_flag[wo] = 2;
        printf("Total number of nodes (%s) without %d in %s is %llu\n", TYPE, wo%5, infile, num_nodes);
    }
    /*  mark nodes in use and GC unmarked */
    unmark_forest(&F);
    for (int i=0; i<5*(argc-1); i++) {
        // printf("root flag %d: %d\n", i, root_flag[i]);
        if (root_flag[i]==1) mark_nodes(&F, root_edge[i].target);
    }
    gc_unmarked(&F);
    /*  clear flags and set root edge to terminal, then mark for counting */
    unmark_forest(&F);
    for (int i=0; i<5*(argc-1); i++) {
        if (root_flag[i]==2) {
            root_edge[i].target = rexdd_make_terminal(dc_val);
            // root_flag[i] = 0;
        } else if (root_flag[i]==1) {
            mark_nodes(&F, root_edge[i].target);
        }
    }
    if (argc > 2) {
        num_nodes = 0;
        for (q=0; q<F.M->pages_size; q++) {
            const rexdd_nodepage_t *page = F.M->pages+q;
            for (n=0; n<page->first_unalloc; n++) {
                if (rexdd_is_packed_marked(page->chunk+n)) {
                    num_nodes++;
                }
            } // for n
        } // for p
        printf("Total number of nodes (%s) in forest removing one root in each group is %llu\n", TYPE, num_nodes);
    }
    printf("----------------------------------------------------------------------------\n");

/*--------------------------------------------Concretizing----------------------------------------------*/
    int ordering[NUM_OUT] = {0, 1, 2, 3, 4};
    /* adding paths to each root with outcome value 0 */
    num_inputbits = 0;
    num_f = 0;
    index = 0;
    count_gc = 0;

    start_time = clock();
    for (int k=1; k<argc; k++) {
        infile = argv[k];

        // now let's read minterms
        char fmt = 0 , comp = 0;
        char type[2];
        file_type(infile, type);
        fmt = type[0];
        comp = type[1];
        
        parser p;
        file_reader fr;
        init_file_reader(&fr, infile, comp);
        init_parser(&p, &fr);
        switch (fmt) {
            case 'p':
            case 'P':
                read_header_pla(&p);
                break;        
            case 'b':
            case 'B':
                read_header_bin(&p);
                break;
            default:
                printf("No parser for format %c\n",fmt);
                // return 0;
        }

        num_inputbits = p.inbits;
        num_f = p.numf;
        char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level

        char*** minterms;
        minterms =  init_minterms(NUM_OUT, BUF_SIZE, num_inputbits);
        unsigned minterms_state[NUM_OUT];
        int out_index;
        for (out_index=0; out_index<NUM_OUT; out_index++) {
            minterms_state[out_index] = 0;
        }

        index = 0;
        count_gc = 0;
        /*
        *  Reading minterms and building forest
        */
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p, inputbits, &term)) break;
            }
            // set the path on corresponding location
            index = term - '1' + 5*(k-1);

            add(minterms_state, minterms, num_inputbits, inputbits, term - '1');

            // check if reach the buffer size, and union minterms buffer
            for (out_index = 0; out_index<NUM_OUT; out_index++) {
                if (minterms_state[out_index] == BUF_SIZE) {
                    if (root_flag[out_index+5*(k-1)] == 2) {
                        for (int i=0+5*(k-1); i<5+5*(k-1); i++) {
                            if (root_flag[i] == 1) {
                                root_edge[i] = union_minterms(&F, num_inputbits, &root_edge[i], minterms[out_index], 0, minterms_state[out_index]);
                            }
                        }
                    } else if (root_flag[out_index+5*(k-1)] == 1) {
                        for (int i=0; i<NUM_OUT; i++) {
                            if (ordering[i] != out_index && root_flag[ordering[i]+5*(k-1)] == 1) {
                                root_edge[ordering[i]+5*(k-1)] = union_minterms(&F, num_inputbits, &root_edge[ordering[i]+5*(k-1)], minterms[out_index], 0, minterms_state[out_index]);
                            } else if (ordering[i] == out_index) break;
                        }
                    }
                    minterms_state[out_index] = 0;
                }
            }
            
            count_gc++;
            if (count_gc % (num_f / 5) == 0) {
                unmark_forest(&F);
                for (int i=0; i<5*(argc-1); i++) {
                    mark_nodes(&F, root_edge[i].target);
                }
                gc_unmarked(&F);
                printf("GC (%d / %d): %d done\n", k, argc-1, (int)(count_gc / (num_f / 5)));
            }
        }
        // flush remaining minterms
        for (out_index = 0; out_index<NUM_OUT; out_index++) {
                if (minterms_state[out_index] > 0) {
                    if (root_flag[out_index+5*(k-1)] == 2) {
                        for (int i=0+5*(k-1); i<5+5*(k-1); i++) {
                            if (root_flag[i] == 1) {
                                root_edge[i] = union_minterms(&F, num_inputbits, &root_edge[i], minterms[out_index], 0, minterms_state[out_index]);
                            }
                        }
                    } else if (root_flag[out_index+5*(k-1)] == 1) {
                        for (int i=0; i<NUM_OUT; i++) {
                            if (ordering[i] != out_index && root_flag[ordering[i]+5*(k-1)] == 1) {
                                root_edge[ordering[i]+5*(k-1)] = union_minterms(&F, num_inputbits, &root_edge[ordering[i]+5*(k-1)], minterms[out_index], 0, minterms_state[out_index]);
                            } else if (ordering[i] == out_index) break;
                        }
                    }
                    minterms_state[out_index] = 0;
                }
            }

        free_parser(&p); // file reader will be free
        free_minterms(minterms, NUM_OUT, BUF_SIZE, num_inputbits);
        fp = fopen("concretizing_process.txt", "w+");
        fprintf(fp, "Terminal 0: %d / %d\n", k, argc-1);
        fclose(fp);
    }
    end_time = clock();
    printf("** preparing terminal 0 time: %0.2f seconds **\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);

    /* starting concretizing*/
    for (int i=0; i<5*(argc-1); i++) {

        if (root_flag[i]==1) root_edge[i] = osm(&F, &root_edge[i], F.S.num_levels);
        fp = fopen("concretizing_process.txt", "w+");
        fprintf(fp,"Concretizing edge: %d / %d\n", i, 5*(argc-1));
        fclose(fp);
    }

    for (int i=1; i<argc; i++) {
        unmark_forest(&F);
        for (int j=0+5*(i-1); j<5+5*(i-1); j++) {
            if (root_flag[j]==1) mark_nodes(&F, root_edge[j].target);
        }
        num_nodes = 0;
        for (q=0; q<F.M->pages_size; q++) {
            const rexdd_nodepage_t *page = F.M->pages+q;
            for (n=0; n<page->first_unalloc; n++) {
                if (rexdd_is_packed_marked(page->chunk+n)) {
                    num_nodes++;
                }
            } // for n
        } // for p
        printf("number of nodes (%s) in forest %d after concretizing is %llu\n", TYPE, i, num_nodes);
    }
    if (argc>2) {
        unmark_forest(&F);
        for (int i=0; i<5+5*(argc-1); i++) {
            if (root_flag[i] == 1) mark_nodes(&F, root_edge[i].target);
        }
        num_nodes = 0;
        for (q=0; q<F.M->pages_size; q++) {
            const rexdd_nodepage_t *page = F.M->pages+q;
            for (n=0; n<page->first_unalloc; n++) {
                if (rexdd_is_packed_marked(page->chunk+n)) {
                    num_nodes++;
                }
            } // for n
        } // for p
        
        printf("Total number of nodes (%s) in after concretizing is %llu\n", TYPE, num_nodes);
    }

    // /* verifacation */
    // infile = argv[1];
    // char fmt = 0 , comp = 0;
    // char type[2];
    // file_type(infile, type);
    // fmt = type[0];
    // comp = type[1];

    // file_reader fr1;
    // init_file_reader(&fr1, infile, comp);
    // parser p;

    // init_parser(&p, &fr1);
    // switch (fmt) {
    //     case 'p':
    //     case 'P':
    //         read_header_pla(&p);
    //         break;        
    //     case 'b':
    //     case 'B':
    //         read_header_bin(&p);
    //         break;
    //     default:
    //         printf("No parser for format %c\n",fmt);
    //         // return 0;
    // }

    // // printf("Building forest %d for %s\n", n, infile);
    // printf("\tThe format is %c\n", fmt);
    // printf("\tThe compress is %c\n", comp);
    // printf("\tThe number of inbits is %u\n", p.inbits);
    // printf("\tThe number of outbits is %u\n", p.outbits);
    // printf("\tThe number of minterms is %lu\n", p.numf);

    // index = 0;
    // count_gc = 0;
    // bool inputbits_bol[num_inputbits + 2];
    // char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level

    // for (;;) {
    //     if (fmt == 'p') {
    //         if (!read_minterm_pla(&p, inputbits, &term)) break;
    //     } else {
    //         if (!read_minterm_bin(&p, inputbits, &term)) break;
    //     }
    //     // set the path on corresponding location
    //     index = term - '1';
    //     for (unsigned i=0; i< p.inbits+2; i++){
    //         if (inputbits[i] == '1') {
    //             // inputbits_bol[p1.inbits+1-i] = 1;
    //             inputbits_bol[i] = 1;
    //         } else {
    //             // inputbits_bol[p1.inbits+1-i] = 0;
    //             inputbits_bol[i] = 0;
    //         }
    //         // fprintf(mout,"%d ", inputbits_bol[i]);
    //     }

    //     if (root_flag[index] == 2) {
    //         for (int i=0; i<5; i++) {
    //             if (root_flag[i] == 1) {
    //                 if (rexdd_eval(&F,&root_edge[i],F.S.num_levels, inputbits_bol)!=1) {
    //                     continue;
    //                 } else {
    //                     printf("DC minterm evaluation error\n");
    //                     exit(1);
    //                 }
    //             }
    //         }
    //     } else if (root_flag[index] == 1) {
    //         if (rexdd_eval(&F,&root_edge[index],F.S.num_levels, inputbits_bol)!=1) {
    //             printf("minterm value %d evaluation 1 error with term %d\n", index, rexdd_eval(&F,&root_edge[index],F.S.num_levels, inputbits_bol));
    //             exit(1);
    //         }
    //         if (index > 0) {
    //             for (int i=0; i<index; i++) {
    //                 if (root_flag[i] == 1) {
    //                     if (rexdd_eval(&F,&root_edge[i],F.S.num_levels, inputbits_bol)!=1) {
    //                         continue;
    //                     } else {
    //                         printf("minterm value %d evaluation 2 error\n", index);
    //                         exit(1);
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
    // printf("verif pass!\n");
    // free_parser(&p); // file reader will be free
    
    printf("============================================================================\n");
    rexdd_free_forest(&F);
    return 0;
}
