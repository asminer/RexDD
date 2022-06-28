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

typedef rexdd_forest_settings_t* rexdd_forest_settings_p;



/**
    Fill in defaults for the struct of forest settings.
    This makes it easier to modify one or two settings
    and use defaults for the rest.

    @param  L   Number of variables.
    @param  s   Settings; will be overwritten (unless null).

 */
void rexdd_default_forest_settings(
        uint_fast32_t L,
        rexdd_forest_settings_p s);



/********************************************************************
 *
 *  BDD Forest.
 *
 ********************************************************************/

struct rexdd_forest_s {
    rexdd_forest_settings_t S;
    rexdd_nodeman_t M;
    rexdd_unique_table_t UT;

    // ...

    struct rexdd_function_s *roots;
};

typedef struct rexdd_forest_s   rexdd_forest_t;
typedef struct rexdd_forest_s*  rexdd_forest_p;


/**
 *  Initialize a forest.
 *      @param  F       Forest to initialize.
 *      @param  s       Pointer to forest settings to use.
 */
void rexdd_create_forest(
        rexdd_forest_p F,
        const rexdd_forest_settings_p s);

/**
 *  Free all memory used by a forest.
 *      @param  F           Pointer to forest struct
 */
void rexdd_destroy_forest(
        rexdd_forest_p F);


/**
 * @brief Normalize the four equivalent forms
 *      @param  P       Desired target node, unpacked
 *      @param  out     Normalized edge will be written here
 * 
 */
void rexdd_normalize_edge(
        rexdd_unpacked_node_t   P,
        rexdd_edge_t            *out);


/**
 *  Reduce an edge.
 *      @param  F       Forest for the edge.
 *      @param  n       Level number
 *      @param  l       Desired edge labels; might not be possible
 *      @param  p       Desired target node, unpacked
 *      @param  out     Reduced edge will be written here.
 */
void rexdd_reduce_edge(
        rexdd_forest_p          F,
        uint_fast32_t           n,
        rexdd_edge_label_t      l,
        rexdd_unpacked_node_t   p,
        rexdd_edge_t            *out);



/********************************************************************
 *
 *  Top-level, easy to use function interface.
 *
 ********************************************************************/

struct rexdd_function_s {

    rexdd_forest_p owner;

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
        rexdd_forest_p      For,
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
