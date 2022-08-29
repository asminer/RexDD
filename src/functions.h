#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "forest.h"

/********************************************************************
 *
 *  Top-level, easy to use function interface.
 *
 ********************************************************************/

struct rexdd_function_s {
    const char* name;      // used for displaying in dot output
    rexdd_forest_t *owner;

    rexdd_edge_t root;

    struct rexdd_function_s* prev;
    struct rexdd_function_s* next;
};

typedef struct rexdd_function_s     rexdd_function_t;


/****************************************************************************
 *
 *  Initialize a function struct.
 *  Sets critical members to null.
 *
 */
static inline void rexdd_init_function(rexdd_function_t *f)
{
    rexdd_check1(f, "null pointer for f\n");

    f->name = 0;
    f->owner = 0;
}


/****************************************************************************
 *
 *  Done with a function.
 *  Will remove the function from the forest's list of root nodes,
 *  and zero out the struct.
 *
 */
void rexdd_done_function(rexdd_function_t *f);


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
        bool                c);



/****************************************************************************
 *
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
        const rexdd_function_t *f,
        const rexdd_function_t *g,
        const rexdd_function_t *h,
              rexdd_function_t *result);



#endif
