#ifndef FOREST_H
#define FOREST_H

#include "unpacked.h"
#include "nodeman.h"
#include "unique.h"
#include "error.h"
#include "computing.h"
#include <stdlib.h>

#define REXBDD  0
#define QBDD    1
#define CQBDD   2
#define SQBDD   3
#define CSQBDD  4
#define FBDD    5
#define CFBDD   6
#define SFBDD   7
#define CSFBDD  8
#define ZBDD    9
#define ESRBDD  10
#define CESRBDD 11

/********************************************************************
 *
 *  Forest settings.
 *
 *  There are not many settings for now, but to keep the
 *  interface stable, all forest settings should be incorporated
 *  into this structure.
 *
 *  Current settings:
 *
 *      num_levels:     number of variables.  In the struct for
 *                      consistency.
 *
 *  Probable additions:
 *
 *      terminal_type:  booleans, integers, reals, or custom?
 *
 *      switching on/off various rules, complement bits, swap bits
 *
 ********************************************************************/

typedef struct {
    uint_fast32_t num_levels;
    char bdd_type;      // BDD type default RexBDD: 0;
    char* type_name;

    // TBD
} rexdd_forest_settings_t;
// global setting BDD type
void rexdd_type_setting(rexdd_forest_settings_t* s, char type);


/**
    Fill in defaults for the struct of forest settings.
    This makes it easier to modify one or two settings
    and use defaults for the rest.

    @param  L   Number of variables.
    @param  s   Settings; will be overwritten (unless null).

 */
void rexdd_default_forest_settings(
        uint_fast32_t L,
        rexdd_forest_settings_t *s);



/********************************************************************
 *
 *  BDD Forest.
 *
 ********************************************************************/

struct rexdd_forest_s {
    rexdd_forest_settings_t S;
    rexdd_nodeman_t *M;
    rexdd_unique_table_t *UT;
    rexdd_comp_table_t *CT;

    uint64_t num_ops;
    uint64_t ct_hits;

    uint64_t num_nots;
    uint64_t ct_hits_nots;

    // ...

    // declared in functions.h
    struct rexdd_function_s *roots;
};

typedef struct rexdd_forest_s   rexdd_forest_t;



/**
 *  Initialize a forest.
 *
 *      @param  F       Forest to initialize.
 *      @param  s       Pointer to forest settings to use.
 */
void rexdd_init_forest(
        rexdd_forest_t *F,
        const rexdd_forest_settings_t *s);

/**
 *  Free all memory used by a forest.
 *
 *      @param  F           Pointer to forest struct
 */
void rexdd_free_forest(
        rexdd_forest_t *F);


/**
 *  Normalize unpacked node "*P".  Edge "*out", which specifies an incoming
 *  edge rule (ignored), a swap bit, a complement bit, and a target (ignored)
 *  is only used to return the swap bit and the complement bit, which may be
 *  changed by the normalization.
 *
 *      If the node "*P" needs swap, the swap bit of edge "*out" will be
 *      set to 1, the node "*P" will be swapped;
 *      If the node "*P" needs complement, the complement bit of edge "*out"
 *      will be set to 1, the node "*P" will be complemented.
 *
 *      @param  F       Rexdd Forest   NOT NEEDED NOW, BUT MAYBE IN THE FUTURE???
 *      @param  P       Desired unpacked node waiting for normalization
 *      @param  out     Normalized edge label will be written here
 *
 */
void rexdd_normalize_node(
        rexdd_forest_t          *F,
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *out);


/**
 *  Reduce unpacked node "*P" by checking the forbidden patterns
 *  of nodes with both edges to terminal 0 (terminal patterns)
 *  and with at most one edge to terminal 0 *  (nonterminal patterns).
 *  Edge "*reduced" can not be null, it will be written the long edge
 *  that represent unpacked node "*P".
 *
 *      If the unpacked node "*P" is a pattern node, the long edge
 *      written into "*reduced" will target to one child of node "*P"
 *      (depends on the different patterns), and store the corresponding
 *      rule, swap bit, complement bit;
 *
 *      If the unpacked node "*P" is not a pattern node, it will be
 *      normalized by calling rexdd_normalize_node function, which
 *      can set the swap bit and complement bit of edge "*reduced".
 *      Then node "*P" will get a node handle in the unique table by
 *      calling rexdd_nodeman_get_handle and rexdd_insert_UT to check
 *      if this node is duplicated and insert it into the unique table.
 *
 *      @param  F       Rexdd Forest
 *      @param  P       The unpacked node waiting for reduction
 *      @param  reduced Reduced edge will be written here
 *
 */
void rexdd_reduce_node(
        rexdd_forest_t          *F,
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *reduced);


/**
 *  Merge the incoming edge with label "l", which is respect of
 *  level m, target to the node at level n; edge "*reduced" <rho, s, c, q>
 *  represents the node at level n after calling rexdd_reduce_node
 *  function. Edge "*out" can not be null, it will be written the
 *  merged edge.
 *
 *      If it is compatible merge, the merged edge written
 *      into "*out" will target to the target node of "*reduced",
 *      and store the corresponding rule, swap bit, complement bit;
 *
 *      (Push up one)
 *      If it is incompatible merge, the incoming edge label "l" rule
 *      is not rexdd_rule_AL or rexdd_rule_AH, the merged edge written
 *      into "*out" will target to a new node created at level n+1, and
 *      store the corresponding rule, swap bit, complement bit; the node
 *      q will be one child of the new node with corresponding edges.
 *      The new node will be normalized, then check and insert
 *      into the unique table by calling rexdd_insert_UT function.
 *
 *      (Push up all)
 *      If it is incompatible merge, the incoming edge label "l" rule
 *      is rexdd_rule_AL or rexdd_rule_AH, there will be (m - q.level)
 *      or (m - q.level + 1) new nodes created from level q.level+1 to m.
 *      The merged edge written into "*out" will target to the new node
 *      at level m with rule rexdd_rule_X and corresponding swap bit,
 *      complement bit.
 *
 *      @param  F       RexDD Forest
 *      @param  m       The represent level of the incoming edge
 *      @param  n       The target node level of the incoming edge
 *      @param  l       The incoming edge label
 *      @param  reduced The reduced edge
 *      @param  out     Merged edge will be written here
 *
 */
void rexdd_merge_edge(
        rexdd_forest_t          *F,
        uint32_t                m,
        uint32_t                n,
        rexdd_edge_label_t      l,
        rexdd_edge_t            *reduced,
        rexdd_edge_t            *out);


/**
 *  Reduce the incoming edge with label "l", which is respect of
 *  level m, target to the unpacked node "p"; edge "*out" can not
 *  be null, it will be written the reduced edge.
 *
 *      @param  F       RexDD Forest
 *      @param  m       The represent level of the incoming edge
 *      @param  l       Desired edge labels; might not be possible
 *      @param  p       Desired target node, unpacked
 *      @param  out     Reduced edge will be written here.
 */
void rexdd_reduce_edge(
        rexdd_forest_t          *F,
        uint_fast32_t           m,
        rexdd_edge_label_t      l,
        rexdd_unpacked_node_t   p,
        rexdd_edge_t            *out);

/**
 *  Evaluate the function
 *
 *  Evaluate the function f^m_<rexdd_edge>(vars[]) by the edge with
 *  respect of level m on vars.
 *
 *      For example, edge <EL0, 0, 1, P> of level m has rule EL0,
 *      swapped bit 0, complement bit 0 and target node P. Then
 *      condition
 *              P.level < m < forest setting level L
 *      must be met; More specifically, if this edge skips 1 node,
 *      m must be P.level + 1; if this edge skips k>1 nodes, m must
 *      be P.level + k.
 *
 *      and
 *              vars[] must be indexed from 0 to L (vars[0] is not
 *              used); vars[i] is represented the variable of node
 *              at level i
 *
 *      @param  F       Rexdd Forest
 *      @param  e       The edge with respect of level m
 *      @param  m       The represent level of the edge
 *      @param  vars    The given variables from nodes of each level
 *
 */
bool rexdd_eval(
        rexdd_forest_t          *F,
        rexdd_edge_t            *e,
        uint32_t                m,
        bool                    vars[]);

/****************************************************************************
 *
 *  Create a dot file for the forest,
 *  with the given root edge.
 *      TBD: use forest roots instead
 *      TBD: allow names to be attached to functions
 *      TBD: only display root edges with names?
 *      TBD: only display nodes reachable from a root edge?
 *
 *      @param  out         Output stream to write to
 *      @param  F           Forest to use.
 *
 *      @param  e           Root edge, for now.
 *
 */
void rexdd_export_dot(FILE* out, const rexdd_forest_t *F, rexdd_edge_t e);

#endif
