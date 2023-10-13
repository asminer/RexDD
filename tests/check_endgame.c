#include "rexdd.h"

#include <assert.h>
#include <time.h>

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
        // bool inputbits_bol[num_inputbits + 2];

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
        // int index = 0;
        // rexdd_edge_t union_edge;
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p, inputbits, &term)) break;
            }
            // set the path on corresponding location
            var_index = bin2dec(inputbits, num_inputbits);
            // index = term - '1' + 5*(n-1);
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
                // get function from all
                edge_function[i] = (function[i]==((j%5)+1))?1:0;
            }
            function_2_edge(&F,edge_function,&root_edge[j],num_inputbits, 0, rows-1);
        }

        free_parser(&p); // file reader will be free
        // printf("Evaling...%c\n",fmt);
        // file_reader fr1;
        // init_file_reader(&fr1, infile, comp);
        // parser p1;
        // init_parser(&p1, &fr1);
        // switch (fmt) {
        //     case 'p':
        //     case 'P':
        //         read_header_pla(&p1);
        //         break;        
        //     case 'b':
        //     case 'B':
        //         read_header_bin(&p1);
        //         break;
        //     default:
        //         printf("No parser for format %c\n",fmt);
        //         // return 0;
        // }

        // // FILE* mout;
        // // mout = fopen("minterms.txt", "w");
        // for (;;) {
        //     if (fmt == 'p') {
        //         if (!read_minterm_pla(&p1, inputbits, &term)) break;
        //     } else {
        //         if (!read_minterm_bin(&p1, inputbits, &term)) {
        //             printf("break!\n");
        //             break;
        //         }
        //     }
        //     index = term - '1' + 5*(n-1);
        //     for (unsigned i=0; i< p1.inbits+2; i++){
        //         if (inputbits[i] == '1') {
        //             // inputbits_bol[p1.inbits+1-i] = 1;
        //             inputbits_bol[i] = 1;
        //         } else {
        //             // inputbits_bol[p1.inbits+1-i] = 0;
        //             inputbits_bol[i] = 0;
        //         }
        //         // fprintf(mout,"%d ", inputbits_bol[i]);
        //     }
        //     // fprintf(mout, "\t%d\n", index);
        //     //evaling every root
        //     bool is_right = 1;
        //     int r = 0;
        //     for (r=0+5*(n-1); r<5+5*(n-1); r++) {
        //         if(r!=index) {
        //             if(!rexdd_eval(&F,&root_edge[r], p1.inbits, inputbits_bol)) continue;
        //             is_right = 0;
        //             break;
        //         } else {
        //             if (rexdd_eval(&F, &root_edge[index], p1.inbits, inputbits_bol)) continue;
        //             is_right = 0;
        //             printf("\tnot right in THE root\n");
        //             break;
        //         }
        //     }
        //     // if (rexdd_eval(&F, &DC_edge[n-1], p1.inbits, inputbits_bol)) is_right = 0;
        //     if (!is_right) {
        //         printf("eval test fail at %d!\n", r-5*(n-1));
        //         // fclose(mout);
        //         free_parser(&p1); // file reader will be free
        //         return 1;
        //     }
        // }
        // // fclose(mout);
        // printf("Evaluation pass!\n\n");
        // free_parser(&p1); // file reader will be free

        free(function);
        free(edge_function);
    } // end of files for loop
    end_time = clock();
    printf("** reading and building time: %0.2f seconds **\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);

    // unmarking forest
    unmark_forest(&F);
    // marking nodes in use
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
        printf("Total number of nodes (%s) in %s is %llu\n", F.S.type_name, infile, num_nodes);
    } else {
        printf("Total number of nodes (%s) in forest is %llu\n", F.S.type_name, num_nodes);
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
    printf("Total number of nodes (%s) without %d is %llu\n", F.S.type_name, wo, num_nodes);
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
