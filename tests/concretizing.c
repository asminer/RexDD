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
        rexdd_set_edge(&root_edge[i], rexdd_rule_X, 0, 0, rexdd_make_terminal(0));
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
    printf("============================================================================\n");
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

    /*--------------------------------------------Verifacation----------------------------------------------*/
    infile = argv[1];
    char fmt = 0 , comp = 0;
    char type[2];
    file_type(infile, type);
    fmt = type[0];
    comp = type[1];

    file_reader fr1;
    init_file_reader(&fr1, infile, comp);
    parser p;

    init_parser(&p, &fr1);
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

    // printf("Building forest %d for %s\n", n, infile);
    printf("\tThe format is %c\n", fmt);
    printf("\tThe compress is %c\n", comp);
    printf("\tThe number of inbits is %u\n", p.inbits);
    printf("\tThe number of outbits is %u\n", p.outbits);
    printf("\tThe number of minterms is %lu\n", p.numf);

    index = 0;
    count_gc = 0;
    bool inputbits_bol[num_inputbits + 2];
    char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level
    int term_eval = 0;
    clock_t total_time = 0;


    for (;;) {
        if (fmt == 'p') {
            if (!read_minterm_pla(&p, inputbits, &term)) break;
        } else {
            if (!read_minterm_bin(&p, inputbits, &term)) break;
        }
        // set the path on corresponding location
        index = term - '1';
        for (unsigned i=0; i< p.inbits+2; i++){
            if (inputbits[i] == '1') {
                // inputbits_bol[p1.inbits+1-i] = 1;
                inputbits_bol[i] = 1;
            } else {
                // inputbits_bol[p1.inbits+1-i] = 0;
                inputbits_bol[i] = 0;
            }
            // fprintf(mout,"%d ", inputbits_bol[i]);
        }

        start_time = clock();
        // for (int test_num = 0; test_num<100; test_num++) {
            for (term_eval=0; term_eval<5; term_eval++) {
                if (root_flag[term_eval] == 1 && rexdd_eval(&F,&root_edge[term_eval],F.S.num_levels, inputbits_bol) == 1) break;
                continue;
            }
            if (term_eval == 5) {
                for (int i=0; i<5; i++) {
                    if (root_flag[i]==2) {
                        term_eval = i;
                        break;
                    }
                }
            }
        // }
        end_time = clock();
        if (term_eval == index) {
            total_time += (end_time-start_time);
        } else {
            printf("query error!\n");
            exit(1);
        }
        // if (root_flag[index] == 2) {
        //     for (int i=0; i<5; i++) {
        //         if (root_flag[i] == 1) {
        //             if (rexdd_eval(&F,&root_edge[i],F.S.num_levels, inputbits_bol)!=1) {
        //                 continue;
        //             } else {
        //                 printf("DC minterm evaluation error\n");
        //                 exit(1);
        //             }
        //         }
        //     }
        // } else if (root_flag[index] == 1) {
        //     if (rexdd_eval(&F,&root_edge[index],F.S.num_levels, inputbits_bol)!=1) {
        //         printf("minterm value %d evaluation 1 error with term %d\n", index, rexdd_eval(&F,&root_edge[index],F.S.num_levels, inputbits_bol));
        //         exit(1);
        //     }
        //     if (index > 0) {
        //         for (int i=0; i<index; i++) {
        //             if (root_flag[i] == 1) {
        //                 if (rexdd_eval(&F,&root_edge[i],F.S.num_levels, inputbits_bol)!=1) {
        //                     continue;
        //                 } else {
        //                     printf("minterm value %d evaluation 2 error\n", index);
        //                     exit(1);
        //                 }
        //             }
        //         }
        //     }
        // }
    }
    printf("clocks_pre_sec is %lu\n", CLOCKS_PER_SEC);
    printf("Average query time is %0.8f\n", (double)(total_time/CLOCKS_PER_SEC));
    printf("verif pass!\n");
    free_parser(&p); // file reader will be free

    printf("============================================================================\n");
    rexdd_free_forest(&F);
    return 0;
}