
#include "forest.h"

#ifdef FBDD
#   define TYPE "FBDD"
#elif defined QBDD
#   define TYPE "QBDD"
#elif defined ZBDD
#   define TYPE "ZBDD"
#elif defined C_FBDD
#   define TYPE "C_FBDD"
#elif defined C_QBDD
#   define TYPE "C_QBDD"
#elif defined S_FBDD
#   define TYPE "S_FBDD"
#elif defined S_QBDD
#   define TYPE "S_QBDD"
#elif defined CS_FBDD
#   define TYPE "CS_FBDD"
#elif defined CS_QBDD
#   define TYPE "CS_QBDD"
#elif defined ESRBDD
#   define TYPE "ESRBDD"
#elif defined CESRBDD
#   define TYPE "CESRBDD"
#else
#   define TYPE "RexBDD"
#endif

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
 *                                                                          *
 *                              Parser for Endgame                          *
 *                                                                          *
 ****************************************************************************/






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