#include "forest.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// mark the nonterminal nodes from root in the forest *F. This is used for counting the number of nodes
void mark_nodes(
                rexdd_forest_t *F, 
                rexdd_node_handle_t root)
{
    if (!rexdd_is_terminal(root)) {
        if (!rexdd_is_packed_marked(rexdd_get_packed_for_handle(F->M, root))) {
            rexdd_mark_packed(rexdd_get_packed_for_handle(F->M,root));
        }
        rexdd_node_handle_t low, high;
        low = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root));
        high = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root));
        mark_nodes(F, low);
        mark_nodes(F, high);
    }
    
}

// read the number of roots from file *fin, this is used to declar the size of root edges.
unsigned long long read_num_roots(
                FILE *fin)
{
    unsigned long long t = 0;
    char buffer[4];
    while (!feof(fin))
    {
        buffer[0] = skip_whitespace(fin);
        for (int i=1; i<4; i++) {
            buffer[i] = skip_whitespace(fin);
        }
        if (buffer[0] == 'r' && buffer[1] == 'o' && buffer[2] == 'o' && buffer[3] == 't') {
            t = expect_uint64(fin);
            printf ("number of roots: %llu\n", t);
            break;
        }
    }
    return t;
}

// read QBDDs from file *fin, write into forest *F and roots *edges with the total number of roots t.
void read_qbdds(
                FILE *fin,
                rexdd_forest_t *F,
                rexdd_edge_t *edges,
                unsigned long long t)
{
    char type[10];
    int level;
    uint64_t handles;
    rexdd_forest_settings_t s;
    rexdd_node_handle_t *unique_handles;
    uint64_t loc = 0;
    
    rexdd_unpacked_node_t node;
    while (!feof(fin))
    {
        char buffer[4];
        buffer[0] = skip_whitespace(fin);
        for (int i=1; i<4; i++) {
            buffer[i] = skip_whitespace(fin);
        }
        if (buffer[0] == 't' && buffer[1] == 'y' && buffer[2] == 'p' && buffer[3] == 'e'){
            snprintf(type, 10, "QBDD");
        } else if (buffer[0] == 'l' && buffer[1] == 'v' && buffer[2] == 'l' && buffer[3] == 's'){
            level = expect_int(fin);
            s.num_levels = level;
            rexdd_init_forest(F, &s);
        } else if (buffer[0] == 'm' && buffer[1] == 'a' && buffer[2] == 'x' && buffer[3] == 'h'){
            handles = expect_uint64(fin);
            unique_handles = malloc(handles*sizeof(rexdd_node_handle_t));
        } else if (buffer[0] == 'n' && buffer[1] == 'o' && buffer[2] == 'd' && buffer[3] == 'e'){

            loc = expect_uint64(fin) - 1;
            char c = skip_whitespace(fin);
            if (c == 'L') {
                node.level = expect_int(fin);
                if (skip_whitespace(fin) == '0' && skip_whitespace(fin) == '<') {
                    node.edge[0] = expect_edge(fin, unique_handles);
                    if (skip_whitespace(fin) == '1' && skip_whitespace(fin) == '<') {
                        node.edge[1] = expect_edge(fin, unique_handles);
                    }
                }
            }

            *(unique_handles + loc) = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M,&node));

        } else if (buffer[0] == 'r' && buffer[1] == 'o' && buffer[2] == 'o' && buffer[3] == 't') {
            t = expect_uint64(fin);

            printf ("number of roots: %llu\n", t);
            for (uint64_t i=0; i<t; i++) {
                if (skip_whitespace(fin) == '<') {
                    edges[i] = expect_edge(fin, unique_handles);
                }
                // if (i == 3000) break;
            }
            break;
        }
    }
    free(unique_handles);

    fclose(fin);
}

// unmark all nodes in the forest (initializing for counting the marked nodes)
void unmark_forest(
                rexdd_forest_t *F)
{
    uint_fast64_t p, n;
    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                rexdd_unmark_packed(page->chunk+n);
            }
        } // for n
    } // for p
}

/* count the number of nonterminal nodes for a bdd, make sure the forest is unmarked before;
   if bdds of several roots need to be counted, it only requires unmark_forest once before counting. */
unsigned long long count_nodes(
                rexdd_forest_t *F,
                rexdd_node_handle_t root)
{
    // mark nodes for the readin QBDD
    mark_nodes(F, root);

    // count the number of nodes for this QBDD
    unsigned long long count = 0;
    uint_fast64_t p, n;
    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                count++;
            }
        } // for n
    } // for p

    return count;
}

// used to reduce QBDD (no skip edges) to this TYPE of BDDs.
rexdd_edge_t reduce_bdds(
                rexdd_forest_t *F,
                uint_fast32_t m,
                rexdd_edge_label_t l,
                rexdd_unpacked_node_t p)
{
    rexdd_edge_t reduced;
    if (p.level == 1) {
        rexdd_reduce_edge(F, 1, l, p, &reduced);
    } else {
        rexdd_unpacked_node_t new_p, lch, hch;
        new_p.level = p.level;
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M,p.edge[0].target), &lch);
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M,p.edge[1].target), &hch);
        new_p.edge[0] = reduce_bdds(F, m-1, p.edge[0].label, lch);
        new_p.edge[1] = reduce_bdds(F, m-1, p.edge[1].label, hch);
        rexdd_reduce_edge(F, m, l, new_p, &reduced);
    }
    return reduced;
}

// parameter of file path if needed TBD
int main(int argc, const char* const* argv)
{
    FILE *fin;
    // char file_name[64];     // the input file name
    // fin = fopen("THE_INPUT_FILE","r");    // fclose in the function read_qbdds
    fin = fopen("saved_BDD.txt", "r");
    unsigned long long t = read_num_roots(fin), i;
    rexdd_forest_t F_in;
    // if the number of roots is too big, it may need to malloc
    rexdd_edge_t edges[t], reduced;
    fclose(fin);
    fin = fopen("saved_BDD.txt", "r");
    read_qbdds(fin, &F_in, edges, t);

    unsigned long long count = 0;
    unmark_forest(&F_in);

    // test case
    // count = count_nodes(&F_in, edges[3000].target);
    // printf("The total number of nodes for QBDD is %llu\n", count);
    // fin = fopen("read_QBDD.gv", "w+");
    // build_gv(fin,&F_in,edges[3000]);
    // fclose(fin);

    // reduce the BDDs
    rexdd_unpacked_node_t p_root;
    for (i=0; i<t; i++) {
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F_in.M, edges[i].target), &p_root);
        reduced = reduce_bdds(
                    &F_in,
                    rexdd_unpack_level(rexdd_get_packed_for_handle(F_in.M, edges[i].target)),
                    edges[i].label,
                    p_root);
        mark_nodes(&F_in, reduced.target);
    }
    uint_fast64_t p, n;
    for (p=0; p<F_in.M->pages_size; p++) {
        const rexdd_nodepage_t *page = F_in.M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                count++;
            }
        } // for n
    } // for p

    // output the result into a file for each benchmark
    // unmark_forest(&F_in);
    // count = count_nodes(&F_in, reduced.target);
    printf("The total number of nodes for %s is %llu\n", TYPE, count);

    // fin = fopen("read_RexBDD.gv", "w+");
    // build_gv(fin, &F_in, reduced);
    // fclose(fin);

    // free if needed
    // free(edges);
    rexdd_free_forest(&F_in);

    return 0;
}