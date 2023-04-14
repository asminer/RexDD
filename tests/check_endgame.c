#include "forest.h"
#include "parser.h"
#include "helpers.h"

#include <assert.h>

unsigned long bin2dec(char* bins, int length)
{
    if (bins == 0){
        printf("null vars\n");
        return 0;
    }
    unsigned long ans = 0;
    for (int i = 1; i<length+1; i++) {
        ans += (bins[i]=='1')? 0x01UL<<(i-1) : 0;
    }
    return ans;
}

void functionToEdge(rexdd_forest_t* F, char* functions, rexdd_edge_t* root_out, int L, unsigned long start, unsigned long end)
{
    //
    rexdd_unpacked_node_t temp;
    temp.level = L;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;
    /*  terminal case */
    if ((end-start == 1) && (L==1)) {
        // printf("\tterminal case!\n");
        temp.edge[0].target = rexdd_make_terminal(functions[start]);
        temp.edge[1].target = rexdd_make_terminal(functions[end]);
        
        rexdd_reduce_edge(F, L, l, temp, root_out);
        return;
    }
    const int next_lvl = L-1;
    // printf("\t\tgo level %d!\n", next_lvl);
    functionToEdge(F, functions, &temp.edge[0], next_lvl, start, (end-start)/2+start);
    functionToEdge(F, functions, &temp.edge[1], next_lvl, (end-start)/2+start+1, end);
    // printf("\t\treduce level %d!\n", temp.level);
    rexdd_reduce_edge(F, L, l, temp, root_out);
}

uint64_t count_nodes(rexdd_forest_t* F, rexdd_edge_t* root, int dc, int num)
{
    unmark_forest(F);
    for (int i=0; i<num; i++) {
        if (i != dc) {
            mark_nodes(F, root[i].target);
        }
    }
    // mark_nodes(F, root->target);
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
/*=======================================================================================================
 *                                  RexBDD related funcitons
 *=======================================================================================================*/

// build a forest recording the input function (2D array of bool with specific container number)
rexdd_edge_t union_minterm(rexdd_forest_t* F, rexdd_edge_t* root, char* minterm, uint32_t K)
{
    // the final answer
    rexdd_edge_t ans;
    ans.label.rule = rexdd_rule_X;
    ans.label.complemented = 0;
    ans.label.swapped = 0;

    // terminal case 
    if (K==0) {
        rexdd_set_edge(&ans,
                        rexdd_rule_X,
                        0,
                        0,
                        (root->label.complemented) ? rexdd_make_terminal(0) : rexdd_make_terminal(1));
        return ans;
    }

    /*
     * here means the root edge can not capture the minterm!
     *      we need to break root edge if it's a long edge
     */
    uint32_t root_skip;     // if the root edge is a long edge or not
    if (rexdd_is_terminal(root->target)) {
        root_skip = K;
    } else {
        root_skip = K - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, root->target));
    }

    // determine down pointers
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
        assert(root_skip > 0);
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

    if (minterm[K] == '1') {
        tmp.edge[1] = union_minterm(F, &root_down1, minterm, K-1);
        tmp.edge[0] = root_down0;
    } else {
        tmp.edge[0] = union_minterm(F, &root_down0, minterm, K-1);
        tmp.edge[1] = root_down1;
    }

    rexdd_reduce_edge(F, K, ans.label, tmp, &ans);
    return ans;
}


/*=======================================================================================================
 *                                           main function here
 =======================================================================================================*/


int main(int argc, const char* const* argv)
{
    //
    if (argc == 1) {
        printf("Need one input file\n");
        return 0;
    }

    rexdd_forest_t F;
    rexdd_forest_settings_t s;
    rexdd_edge_t root_edge[5*(argc - 1)];
    rexdd_edge_t DC_edge[argc-1];
    // Initializing the root edges and don't care edge
    for (int i=0; i<5*(argc-1); i++) {
        rexdd_set_edge(&root_edge[i], rexdd_rule_X, 0, 0, rexdd_make_terminal(0));
    }
    for (int i=0; i<(argc-1); i++) {
        rexdd_set_edge(&DC_edge[i], rexdd_rule_X, 0, 0, rexdd_make_terminal(0));
    }

    const char* infile = 0;

    for (int n=1; n<argc; n++) {

        infile = argv[n];

        // now let's read minterms
        char fmt = 0 , comp = 0;
        char type[2];
        file_type(infile, type);
        fmt = type[0];
        comp = type[1];

        file_reader fr;
        init_file_reader(&fr, infile, comp);
        parser p;
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

        printf("Building forest %d for %s\n", n, infile);
        printf("\tThe format is %c\n", fmt);
        printf("\tThe compress is %c\n", comp);
        printf("\tThe number of inbits is %u\n", p.inbits);
        printf("\tThe number of outbits is %u\n", p.outbits);
        printf("\tThe number of minterms is %lu\n", p.numf);

        char term;
        unsigned num_inputbits = p.inbits;
        char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level
        bool inputbits_bol[num_inputbits + 2];

        unsigned long rows = 0x01UL<<(num_inputbits);   // may be bigger for 5 pieces
        char *function = malloc(rows*sizeof(char));
        if (!function){
            printf("Unable to malloc for function\n");
            exit(1);
        }
        for (size_t i=0; i<rows; i++) {
            function[i] = 0;
        }

        // Initializing the forest
        if (n == 1) {
            rexdd_default_forest_settings(p.inbits, &s);
            rexdd_init_forest(&F, &s);
        }

        unsigned long var_index = 0;
        int index = 0;
        // rexdd_edge_t union_edge;
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p, inputbits, &term)) break;
            }
            // set the path on corresponding location
            var_index = bin2dec(inputbits, num_inputbits);
            index = term - '1' + 5*(n-1);
            function[var_index] = term-'0';

            // root_edge[index] = union_minterm(&F, &root_edge[index], inputbits, p.inbits);

            // gc TBD here for huge number of nodes
        }
        char* edge_function = malloc(rows*sizeof(char));
        if (!edge_function){
            printf("Unable to malloc for function\n");
            exit(1);
        }
        for (size_t i=0; i<rows; i++) {
            edge_function[i] = 0;
        }

        for (int j=0+5*(n-1); j<5+5*(n-1); j++) {
            for (size_t i=0; i<rows; i++) {
                edge_function[i] = (function[i]==((j%5)+1))?1:0;
            }
            functionToEdge(&F,edge_function,&root_edge[j],num_inputbits, 0, rows-1);
        }

        printf("Done building!\n");

        printf("Evaling...%c\n",fmt);
        free_parser(&p); // file reader will be free
        file_reader fr1;
        init_file_reader(&fr1, infile, comp);
        parser p1;
        init_parser(&p1, &fr1);
        switch (fmt) {
            case 'p':
            case 'P':
                read_header_pla(&p1);
                break;        
            case 'b':
            case 'B':
                read_header_bin(&p1);
                break;
            default:
                printf("No parser for format %c\n",fmt);
                // return 0;
        }

        // FILE* mout;
        // mout = fopen("minterms.txt", "w");
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p1, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p1, inputbits, &term)) {
                    printf("break!\n");
                    break;
                }
            }
            index = term - '1' + 5*(n-1);
            for (unsigned i=0; i< p1.inbits+2; i++){
                if (inputbits[i] == '1') {
                    // inputbits_bol[p1.inbits+1-i] = 1;
                    inputbits_bol[i] = 1;
                } else {
                    // inputbits_bol[p1.inbits+1-i] = 0;
                    inputbits_bol[i] = 0;
                }
                // fprintf(mout,"%d ", inputbits_bol[i]);
            }
            // fprintf(mout, "\t%d\n", index);
            //evaling every root
            bool is_right = 1;
            int r = 0;
            for (r=0+5*(n-1); r<5+5*(n-1); r++) {
                if(r!=index) {
                    if(!rexdd_eval(&F,&root_edge[r], p1.inbits, inputbits_bol)) continue;
                    is_right = 0;
                    break;
                } else {
                    if (rexdd_eval(&F, &root_edge[index], p1.inbits, inputbits_bol)) continue;
                    is_right = 0;
                    printf("\tnot right in THE root\n");
                    break;
                }
            }
            // if (rexdd_eval(&F, &DC_edge[n-1], p1.inbits, inputbits_bol)) is_right = 0;
            if (!is_right) {
                printf("eval test fail at %d!\n", r-5*(n-1));
                // fclose(mout);
                free_parser(&p1); // file reader will be free
                return 1;
            }
        }
        // fclose(mout);
        printf("Evaluation pass!\n\n");
        free_parser(&p1); // file reader will be free

        free(function);
        free(edge_function);
    }

    printf("Unmarking the forest...\n");
    unmark_forest(&F);
    printf("Done unmarking!\n");

    printf("Marking nonterminal nodes in use from roots...\n");
    for (int i=0; i<5*(argc-1); i++) {
        if (rexdd_is_terminal(root_edge[i].target)) {
            printf("\troot %d : %d is T0\n",(i/5)+1, i%5);
        } else {
            printf("\troot %d : %d is %llu\n",(i/5)+1, i%5, root_edge[i].target);
        }
        mark_nodes(&F, root_edge[i].target);
    }
    // printf("\tDC root is %llu\n", (rexdd_is_terminal(DC_edge[argc-2].target))?0:DC_edge[argc-2].target);
    // mark_nodes(&F, DC_edge[argc-2].target);
    printf("Done marking!\n");

    printf("Counting the total number of nodes in forest...\n");
    uint_fast64_t q, n;
    uint64_t num_nodes = 0;
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

    /*  w/o one root edge and count */
    uint64_t min_num_nodes = 0;
    int wo = 0;
    for (int i=0; i<5; i++) {
        min_num_nodes = count_nodes(&F, root_edge, i, 5);
        if (num_nodes > min_num_nodes) {
            num_nodes = min_num_nodes;
            wo = i;
        }
    }
    printf("Total number of nodes (%s) without %d is %llu\n", TYPE, wo, num_nodes);
    printf("============================================================================\n");
    // unmark_forest(&F);
    // for (int i=0; i<5; i++) {
    //     if (i!=wo) mark_nodes(&F, root_edge[i].target);
    // }
    // // mark_nodes(&F, root_edge[wo].target);

    // FILE* fout;
    // fout = fopen("forest.txt", "w+");
    // save(fout, &F, root_edge, 5*(argc-1));
    // fclose(fout);
    rexdd_free_forest(&F);
    return 0;
}
