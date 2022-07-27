#ifndef FOREST_H
#define FOREST_H

#include "unpacked.h"
#include "nodeman.h"
#include "unique.h"
#include "error.h"

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

    // TBD
} rexdd_forest_settings_t;




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

    // ...

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


//---------------------TBD not used for now-----------------------------
/**
 *  Push up one node for the specific merge case
 *      @param  F       Forest for the edge
 *      @param  n       The leve of the target node
 *      @param  l       The incoming edge label
 *      @param  reduced The reduced edge
 *      @param  out     The result edge will be written here
 * 
 */
// void rexdd_puo_edge(
//         uint32_t                n,
//         rexdd_edge_label_t      l,
//         rexdd_edge_t            *reduced,
//         rexdd_edge_t            *out);

//----------------------TBD Not used for now---------------------------

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

/********************************************************************
 *
 *  Top-level, easy to use function interface.
 *
 ********************************************************************/

struct rexdd_function_s {

    rexdd_forest_t *owner;

    rexdd_edge_t root;

    struct rexdd_function_s* prev;
    struct rexdd_function_s* next;
};

typedef struct rexdd_function_s     rexdd_function_t;
typedef struct rexdd_function_s*    rexdd_function_p;

/*
 *  TBD: probably don't make this inlined
 *
 *  Add a function node to its owner's list of roots.
 *
 */
static inline void rexdd_add_to_forest_roots(rexdd_function_p f)
{
    if (0==f) rexdd_error(__FILE__, __LINE__, "null pointer for f\n");

    // TBD.
    // Add node f to the front of f->owner's list of roots,
    // and update doubly-linked list pointers.
}

/*
 *  TBD: probably don't make this inlined
 *
 *  Remove a function node from its owner's list of roots,
 *  and set the owner and list pointers to null.
 *
 */
static inline void rexdd_remove_from_forest_roots(rexdd_function_p f)
{
    if (0==f) rexdd_error(__FILE__, __LINE__, "null pointer for f\n");

    //
    // TBD.
    // update f->prev, f->next, and other pointers.
    // Might need to update f->owner's list pointer.

    f->owner = 0;
    f->prev = 0;
    f->next = 0;
}


/*
 *  Initialize a function struct.
 *
 *  TBD: probably should throw a runtime error if f is null.
 */
static inline void rexdd_init_function(rexdd_function_p f)
{
    if (0==f) rexdd_error(__FILE__, __LINE__, "null pointer for f\n");

    f->owner = 0;
    f->prev = 0;
    f->next = 0;
}


/*
 * Done with a function.
 */
static inline void rexdd_done_function(rexdd_function_p f)
{
    if (0==f) rexdd_error(__FILE__, __LINE__, "null pointer for f\n");

    rexdd_remove_from_forest_roots(f);
}



/*
 *  Build a function of a single variable,
 *  using an already initialized function struct.
 *      @param  fn      function struct, specifies forest to use
 *                      and the function will be updated in place.
 *      @param  v       Variable level.  If 0, will build
 *                      a constant function 'false'.
 *      @param  c       If true, complement the variable
 *                      (i.e., build !v instead of v).
 */
void rexdd_reset_as_variable(
        rexdd_function_p    fn,
        uint32_t            v,
        bool                c);


/*
 *  Initialize a function struct,
 *  and fill it with a function of a single variable.
 *      @param  fn      uninitialized function struct on input;
 *                      will be initialized and filled with the
 *                      desired function.
 *      @param  For     Forest to use.
 *      @param  v       Variable level.  If 0, will build
 *                      a constant function 'false'.
 *      @param  c       If true, complement the variable
 *                      (i.e., build !v instead of v).
 */
void rexdd_init_as_variable(
        rexdd_function_p    fn,
        rexdd_forest_t      *For,
        uint32_t            v,
        bool                c);



/*
 *  If-then-else operator.
 *      @param  f       Function f, already set.
 *      @param  g       Function g, already set, in same forest as f.
 *      @param  h       Function h, already set, in same forest as f.
 *      @param  result  In: initialized function struct.
 *                      Out: result of ITE(f, g, h), which is a new
 *                      function in the same forest as f, g, h, where
 *                      result(x) = f(x) ? g(x) : h(x).
 */
void rexdd_ITE(
        const rexdd_function_p f,
        const rexdd_function_p g,
        const rexdd_function_p h,
              rexdd_function_p result);


#endif
