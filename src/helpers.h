#ifndef HELPER_H
#define HELPER_H

#include "forest.h"
#include <assert.h>


/****************************************************************************
 *
 *  Set a boolean array *count to tell how many nodes in the RexBDD root from 
 *  handle.
 * 
 *  Note: this is not good enough, since the length of *count could be too long
 *          (TBD optimize it with another structure)
 *
 */
void boolNodes(
        rexdd_forest_t *F,
        rexdd_node_handle_t handle,
        bool *count);

/****************************************************************************
 *
 *  This is used to build the dot file for BDD with one root edge, by calling 
 *  function boolNodes to fprint all nodes.
 * 
 *  Note: after optimazation of boolNodes, this function needs to be changed...
 *
 */
void fprint_rexdd(
        FILE *f,
        rexdd_forest_t *F,
        rexdd_edge_t e);

/****************************************************************************
 *
 *  Export a dot file for BDD with one root edge
 * 
 *  A dot file that plots the BDD from root edge e would be exported to the FILE 
 *  *f.
 * 
 */
void build_gv(
        FILE *f,
        rexdd_forest_t *F,
        rexdd_edge_t e);

/****************************************************************************
 *
 *  Export a dot file for a forest with its root edges
 * 
 *  A dot file that plots the forest *F would be exported to the FILE *f with 
 *  its root edges *ptr from 0 to size-1
 * 
 */
void build_gv_forest(
        FILE *f,
        rexdd_forest_t *F,
        rexdd_edge_t *ptr,
        int size);

/****************************************************************************
 *
 *  Set a handles array that stores the handles of all nodes from root node handle
 * 
 *  Note: this needs to optimize for any number of levels, the length of array 
 *  nodes would be exponential increase (is that a good idea?)
 * 
 */
void handleNodes(
        rexdd_forest_t *F,
        rexdd_node_handle_t handle,
        rexdd_node_handle_t *nodes);

/****************************************************************************
 *
 *  Count the number of non-terminal nodes from the node handle
 * 
 *  By calling function handleNodes, it would return the number of non-terminal 
 *  nodes rooted from hanlde
 * 
 */
int countNodes(
        rexdd_forest_t *F,
        rexdd_node_handle_t handle);

/****************************************************************************
 *
 *  Count the number of terminal nodes from the node handle
 * 
 *  It would return 0 if there is only terminal 0 node; it would return 1 if 
 *  there is only terminal 1 node; it would return 2 is there are noth terminal 
 *  0 and 1 nodes.
 * 
 */
int countTerm(
        rexdd_forest_t *F,
        rexdd_node_handle_t handle);

/****************************************************************************
 *  Helper functions for check-pointing and reading
 ****************************************************************************/
// skip white space
char skip_whitespace(FILE *fin);
// expect int and uint64
int expect_int(FILE *fin);
uint64_t expect_uint64(FILE *fin);
// expect an edge rule
rexdd_rule_t expect_rule(FILE *fin);
// expect an edge
rexdd_edge_t expect_edge(FILE *fin, rexdd_node_handle_t *unique_handles);
//expect a reduced edge
rexdd_edge_t expect_reduced_edge(FILE *fin, rexdd_edge_t *reduced_edges);

/****************************************************************************
 *
 *  Save the forest *F and its root edges *edges with the number of root edges 
 *  in a text file.
 * 
 */
void save(
        FILE *fout,
        rexdd_forest_t *F,
        rexdd_edge_t edges[],
        uint64_t t);

/****************************************************************************
 *
 *  Read the forest *F and its root edges *edges with the number of root edges 
 *  in a text file.
 * 
 */
void read_bdd(
        FILE *fin,
        rexdd_forest_t *F,
        rexdd_edge_t edges[],
        uint64_t t);

/****************************************************************************
 *
 *  Export the file that counting the number of functions represented by some 
 *  number of nodes, and the number of nodes for some number of functions.
 * 
 */
void export_funsNum(
        rexdd_forest_t F,
        int levels,
        rexdd_edge_t edges[]);
/****************************************************************************
 *  Unmark all nodes in the forest (initializing for counting the marked nodes)
 */
void unmark_forest(
                rexdd_forest_t *F);
/****************************************************************************
 *  Mark the nonterminal nodes from root in the forest *F. This is used for counting the number of nodes
 */
void mark_nodes(
                rexdd_forest_t *F, 
                rexdd_node_handle_t root);

/****************************************************************************
 *  Union a path into forest *F
 */
rexdd_edge_t union_minterm(
                rexdd_forest_t* F,
                rexdd_edge_t* root,
                char* minterm,
                int outcome,
                uint32_t K);

/****************************************************************************
 *  Initialize a 3-D minterms 
 *      Note:   num_out is the number of possible outcome values; 
 *              buf_size is the max number of minterms for one outcome value slot;
 *              num_bits is the bits length of the minterms
 */
char*** init_minterms(int num_out, unsigned buf_size, unsigned num_bits);

/****************************************************************************
 *  Free a 3-D minterms
 */
void free_minterms(char*** minterms, int num_out, unsigned buf_size, unsigned num_bits);

/****************************************************************************
 *  Union N elements of a 2-D minterms buffer into forest *F
 *      Note: with outcome value outcome
 */
rexdd_edge_t union_minterms(
                rexdd_forest_t* F, 
                uint32_t K, 
                rexdd_edge_t* root, 
                char** minterms, 
                int outcome, 
                unsigned int N);

/****************************************************************************
 *  Expand the low (type=0) child edge or high (type=1) child edge 
 *      from level r of a long edge e
 */
rexdd_edge_t rexdd_expand_childEdge(rexdd_forest_t* F, uint32_t r, rexdd_edge_t* e, bool type);

/****************************************************************************
 *  Create a BDD from root_out in forest *F to encode function (0-1 array)
 */
void function_2_edge(
                rexdd_forest_t* F,
                char* functions,
                rexdd_edge_t* root_out,
                int L, 
                unsigned long start,
                unsigned long end);

/****************************************************************************
 *  Create a BDD constant edge to terminal one/zero at level k
 *      note: only for t=0/1; more general TBD
 */
rexdd_edge_t build_constant(rexdd_forest_t *F, uint32_t k, int t);

/****************************************************************************
 *  Create a BDD for a variable at level k
 */
rexdd_edge_t build_variable(rexdd_forest_t *F, uint32_t k);

/****************************************************************************
 *  Compute the cardinality of BDD from root edge at level lvl
 */
long long card_edge(rexdd_forest_t* F, rexdd_edge_t* root, uint32_t lvl);


/****************************************************************************
 * garbage collection of unmarked nodes in forest F
 */
void gc_unmarked(rexdd_forest_t* F);

#endif


// /****************************************************************************
//  * Helper: write an edge in dot format.
//  * The source node, and the ->, should have been written already.
//  *
//  *  @param  out     File stream to write to
//  *  @param  e       edge
//  *  @param  solid   if true, write a solid edge; otherwise dashed.
//  */
// static void write_dot_edge(
//         FILE* out,
//         rexdd_edge_t e,
//         bool solid);

// /****************************************************************************
//  *
//  *  Create a dot file for the forest,
//  *  with the given root edge.
//  *      TBD: use forest roots instead
//  *      TBD: allow names to be attached to functions
//  *      TBD: only display root edges with names?
//  *      TBD: only display nodes reachable from a root edge?
//  *
//  *      @param  out         Output stream to write to
//  *      @param  F           Forest to use.
//  *
//  *      @param  e           Root edge, for now.
//  *
//  */
// void rexdd_export_dot(
//         FILE* out,
//         const rexdd_forest_t *F,
//         rexdd_edge_t e);