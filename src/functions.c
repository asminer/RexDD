
#include "functions.h"

/********************************************************************
 *
 *  Top-level, easy to use function interface.
 *
 ********************************************************************/


/*
 *
 *  Helper: Add a function node to its owner's list of roots.
 *
 */
static inline void rexdd_add_to_forest_roots(rexdd_function_t *f,
        rexdd_forest_t *owner)
{
    rexdd_sanity1(f, "null pointer for f\n");
    rexdd_sanity1(owner, "null pointer for owner\n");

    f->owner = owner;
    // TBD.
    // Add node f to the front of owner's list of roots,
    // and update doubly-linked list pointers.
}

/*
 *  Helper: Remove a function node from its owner's list of roots,
 *
 */
static inline void rexdd_remove_from_forest_roots(rexdd_function_t *f)
{
    rexdd_sanity1(f, "null pointer for f\n");
    if (0==f->owner) return;

    //
    // TBD.
    // update f->prev, f->next, and other pointers.
    // Might need to update f->owner's list pointer.
}


/****************************************************************************
 *
 *  Done with a function.
 *  Will remove the function from the forest's list of root nodes,
 *  and zero out the struct as needed.
 *
 */
void rexdd_done_function(rexdd_function_t *f)
{
    rexdd_check1(f, "null pointer for f\n");

    rexdd_remove_from_forest_roots(f);

    f->owner = 0;
    f->name = 0;
}


/****************************************************************************
 *
 *  Build a function of a single variable,
 *  using an already initialized function struct.
 *      @param  fn      function struct
 *      @param  forest  Forest to use; may be different from fn's forest
 *      @param  v       Variable level.  If 0, will build
 *                      a constant function 'false'.
 *      @param  c       If true, complement the variable
 *                      (i.e., build !v instead of v).
 */
void rexdd_build_variable(
        rexdd_function_t    *fn,
        rexdd_forest_t      *forest,
        uint32_t            v,
        bool                c)
{
    rexdd_check1(fn, "null pointer for fn\n");
    rexdd_check1(forest, "null pointer for forest\n");

    if (forest != fn->owner) {
        rexdd_remove_from_forest_roots(fn);
        rexdd_add_to_forest_roots(fn, forest);
    }

    // TBD: update fn->root
}




/*=======================
|| We ignore for now   ||
=======================*/


// void rexdd_ITE(
//         const rexdd_function_p f,
//         const rexdd_function_p g,
//         const rexdd_function_p h,
//               rexdd_function_p result)
// {

        // make sure f,g,h are not null.
        // make sure f,g,h have the same owner.
        // if result->owner != f->owner, change it
// }



