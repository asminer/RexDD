#ifndef FOREST_H
#define FOREST_H

#include "unpacked.h"
#include "nodeman.h"
#include "unique.h"

/********************************************************************

    Forest settings.

    There are not many settings for now, but to keep the
    interface stable, all forest settings should be incorporated
    into this structure.

    Current settings:

        num_levels:     number of variables.  In the struct for
                        consistency.

    Probable additions:

        terminal_type:  booleans, integers, reals, or custom?

        switching on/off various rules, complement bits, swap bits

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

    BDD Forest.

********************************************************************/

typedef struct {
    rexdd_forest_settings_t S;
    rexdd_nodeman_t M;
    rexdd_unique_table_t UT;

    // ...
} rexdd_forest_t;

typedef rexdd_forest_t* rexdd_forest_p;


/**
 *  Allocate and initialize a forest.
 *      @param  s       Pointer to forest settings to use.
 *      @return         A pointer to a newly allocated forest, or null on error.
 */
rexdd_forest_p rexdd_create_forest(
        const rexdd_forest_settings_p s);

/**
 *  Free all memory used by a forest.
 *      @param  F           Pointer to forest struct
 */
void rexdd_destroy_forest(
        rexdd_forest_p F);


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
        uint_fast_32_t          n,
        rexdd_edge_label_t      l,
        rexdd_unpacked_node_t   p,
        rexdd_edge_t            *out);

#endif
