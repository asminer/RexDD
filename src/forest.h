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
 *  and with at most one edge to terminal 0 *  (nonterminal patterns),
 *  with the original incoming edge
 *  swap bit and complement bit stored in the rexdd_edge *  reduced
 * 
 *      If the unpacked node can be represented by long edge,
 *      eliminate this node and store the long edge in rexdd_edge
 *      reduced; else store a normalized edge with rule rexdd_rule_X,
 *      and target to this node handle in the unique table in 
 *      the rexdd_edge reduced.
 * 
 *      @param  F       Rexdd Forest
 *      @param  handle  Handle of the unpacked node
 *      @param  P       The unpacked node waiting for reduction
 *      @param  reduced Reduced edge will be written here
 * 
 */
void rexdd_reduce_node(
        rexdd_forest_t          *F,
        rexdd_node_handle_t     handle,
        rexdd_unpacked_node_t   *P, 
        rexdd_edge_t            *reduced);


/**
 *  Merge the incoming edge target to the node with the reduced
 *  edge.
 *
 *      The reduced edge stored the long edge that represent the
 *      node after calling rexdd_reduce_node.
 * 
 *  If it is mergeable, the merged edge will be stored in the
 *  rexdd_edge out. If it is not mergeable, there will be a new
 *  node created upon the target node, check and insert it
 *  to the unique table by calling rexdd_normalize_node; then
 *  the rexdd_edge out will store an edge with rule rexdd_rule_X,
 *  swapped bit 0, complement bit 0 and target to this new node
 * 
 *      @param  F       RexDD Forest
 *      @param  m       The represent level of the incoming edge
 *      @param  n       The target node level of the reduced edge
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
 *  Reduce the desired edge to its target node. It will first reduce
 *  its target node by calling rexdd_reduce_node, then merge the edge
 *  with the reduced long edge, finally check and insert node to the
 *  unique table
 * 
 *      @param  F       RexDD Forest
 *      @param  m       The represent level of the incoming edge
 *      @param  n       The target node level of the reduced edge
 *      @param  l       Desired edge labels; might not be possible
 *      @param  p       Desired target node, unpacked
 *      @param  out     Reduced edge will be written here.
 */
void rexdd_reduce_edge(
        rexdd_forest_t          *F,
        uint_fast32_t           m,
        uint_fast32_t           n,
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
