#include "forest.h"
#include "parser.h"
#include "helpers.h"

#include <assert.h>
#include <time.h>

#define DC_VAL 3

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

void build_parser(const char* infile, parser* p)
{
    char fmt = 0 , comp = 0;
    char type[2];
    file_type(infile, type);
    fmt = type[0];
    comp = type[1];

    file_reader fr;
    init_file_reader(&fr, infile, comp);
    init_parser(p, &fr);
    switch (fmt) {
        case 'p':
        case 'P':
            read_header_pla(p);
            break;        
        case 'b':
        case 'B':
            read_header_bin(p);
            break;
        default:
            printf("No parser for format %c\n",fmt);
            // return 0;
    }
    printf("\tThe format is %c\n", fmt);
    printf("\tThe compress is %c\n", comp);
    printf("\tThe number of inbits is %u\n", p->inbits);
    printf("\tThe number of outbits is %u\n", p->outbits);
    printf("\tThe number of minterms is %lu\n", p->numf);
}

/*-----------------------------------------------Restriction--------------------------------------------*/
rexdd_edge_t restr(rexdd_forest_t* F, rexdd_edge_t* root, uint_fast32_t n)
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
    if (rexdd_is_terminal(lc.target) && (rexdd_terminal_value(lc.target) == DC_VAL)) {
        return restr(F, &hc, levelc-1);
    } else if (rexdd_is_terminal(hc.target) && (rexdd_terminal_value(hc.target) == DC_VAL)) {
        return restr(F, &lc, levelc-1);
    }
    // new node
    rexdd_unpacked_node_t tmp;
    tmp.level = levelc;
    tmp.edge[0] = restr(F, &lc, tmp.level-1);
    tmp.edge[1] = restr(F, &hc, tmp.level-1);
    
    /*
     *  how to deal with complement bit? TBD
     */

    rexdd_reduce_edge(F, n, root->label, tmp, &ans);
    
    return ans;
}

/*-----------------------------------------------One side match-----------------------------------------*/
// rexdd_edge_t osm(rexdd_forest_t* F, rexdd_edge_t* root)
// {
//     //
// }

/*-----------------------------------------------Two side match-----------------------------------------*/
// rexdd_edge_t tsm(rexdd_forest_t* F, rexdd_edge_t* root)
// {
//     //
// }

/*=======================================================================================================
 *                                           main function here
 =======================================================================================================*/
int main(int argc, const char* const* argv)
{
    if (argc == 1) {
        printf("Need one input file\n");
        return 0;
    }

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
        printf("\tBuilding forest %d for %s\n", n, infile);
        printf("\tThe format is %c\n", fmt);
        printf("\tThe compress is %c\n", comp);
        printf("\tThe number of inbits is %u\n", p.inbits);
        printf("\tThe number of outbits is %u\n", p.outbits);
        printf("\tThe number of minterms is %lu\n", p.numf);

        num_inputbits = p.inbits;
        num_f = p.numf;
        char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level

        // Initializing the forest
        if (n == 1) {
            rexdd_default_forest_settings(p.inbits, &s);
            rexdd_init_forest(&F, &s);
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
            index = term - '1' + 5*(n-1);
            if (root_flag[index] == 0) root_flag[index] = 1;

            root_edge[index] = union_minterm(&F, &root_edge[index], inputbits, 1, p.inbits);
            
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
        free_parser(&p); // file reader will be free
    } // end of files for loop
    end_time = clock();
    printf("** reading and building time: %0.2f seconds **\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);

/*
 *  At this point, forest is built!
 */

/*----------------------------------------Mark and count ALL root---------------------------------------*/
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
    // for (int i=0; i<5*(argc-1); i++) {
    //     printf("root flag %d is %d\n", i, root_flag[i]);
    // }
    int ordering[5] = {0, 1, 2, 3, 4};
    /* adding paths to each root with outcome value 0 */
    num_inputbits = 0;
    num_f = 0;
    index = 0;
    count_gc = 0;
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

            if (root_flag[index] == 2) {
                // printf("union removed\n");
                for (int i=0+5*(k-1); i<5+5*(k-1); i++) {
                    if (root_flag[i] == 1) {
                        // printf("union removed %d into %d\n", index, i);
                        root_edge[i] = union_minterm(&F, &root_edge[i], inputbits, 0, p.inbits);
                    }
                }
            } else if (root_flag[index] == 1) {
                /*
                 *  adding paths to pre-ordered root with outcome value 0
                 */
                for (int i=0; i<5; i++) {
                    if ((ordering[i]!=index-5*(k-1)) && (root_flag[ordering[i]+5*(k-1)] == 1)) {
                        root_edge[ordering[i]+5*(k-1)] = union_minterm(&F, &root_edge[ordering[i]+5*(k-1)], inputbits, 0, p.inbits);
                    } else if (ordering[i] == index-5*(k-1)) break;
                }
            }
            
            count_gc++;
            if (count_gc % (num_f / 5) == 0) {
                unmark_forest(&F);
                for (int i=0; i<5*(argc-1); i++) {
                    mark_nodes(&F, root_edge[i].target);
                }
                gc_unmarked(&F);
                printf("GC %d: %d done\n", k, (int)(count_gc / (num_f / 5)));
            }
        }
        free_parser(&p); // file reader will be free
    }

    /* starting concretizing*/
    for (int i=0; i<5*(argc-1); i++) {

        if (root_flag[i]==1) root_edge[i] = restr(&F, &root_edge[i], F.S.num_levels);

        // unmark_forest(&F);
        // if (root_flag[i]==1) mark_nodes(&F, root_edge[i].target);
        // num_nodes = 0;
        // for (q=0; q<F.M->pages_size; q++) {
        //     const rexdd_nodepage_t *page = F.M->pages+q;
        //     for (n=0; n<page->first_unalloc; n++) {
        //         if (rexdd_is_packed_marked(page->chunk+n)) {
        //             num_nodes++;
        //         }
        //     } // for n
        // } // for p
        // printf("number of %d : %d after concretizing is %llu\n", i/5, i%5, num_nodes);
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

    // file_type(infile, type);
    // fmt = type[0];
    // comp = type[1];

    // file_reader fr1;
    // init_file_reader(&fr1, infile, comp);

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

    //     if (index == 0) {
    //         if (rexdd_eval(&F,&root_edge[0],F.S.num_levels, inputbits_bol)==1) {
    //             // printf("Evaluation %d pass!\n", index);
    //             continue;
    //         }
    //         printf("Evaluation of 0 error\n");
    //         exit(1);
    //     } else if (index == 1) {
    //         if (rexdd_eval(&F,&root_edge[0],F.S.num_levels, inputbits_bol)==0) {
    //             // printf("Evaluation %d pass!\n", index);
    //             continue;
    //         }
    //         printf("Evaluation of 1 error\n");
    //         exit(1);
    //     }
    // }
    // printf("verif pass!\n");
    // free_parser(&p); // file reader will be free
    
    printf("============================================================================\n");
    rexdd_free_forest(&F);
    return 0;
}
