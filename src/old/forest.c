
#include "rexdd.h"

/********************************************************************

    Forest details.

********************************************************************/

struct rexdd_forest {
    struct rexdd_forest_settings settings;
    struct rexdd_forest_stats stats;

    // internal storage details here
};

/********************************************************************

    Front end.

********************************************************************/

/* ================================================================= */

void rexdd_default_forest_settings(unsigned L, rexdd_forest_settingsp s)
{
    if (s) {
        s->num_levels = L;

        // TBD
    }
}

/* ================================================================= */

rexdd_forestp rexdd_create_forest(const rexdd_forest_settingsp s)
{
    // TBD
    return 0;
}

/* ================================================================= */

const rexdd_forest_statsp rexdd_readable_forest(const rexdd_forestp F)
{
    if (F) return &F->stats;
    return 0;
}

/* ================================================================= */

void rexdd_destroy_forest(rexdd_forestp F)
{
    // TBD
}

/* ================================================================= */

rexdd_edge rexdd_evaluate(rexdd_forestp F, rexdd_edge e,
                const bool* vars)
{
    // TBD
    return 0;
}

/* ================================================================= */

rexdd_edge rexdd_reduce_edge(rexdd_forestp F, rexdd_reduction_rule rule,
        const rexdd_nodep node)
{
    // TBD
    return 0;
}

/* ================================================================= */

void rexdd_get_node(rexdd_forestp F, rexdd_edge e, rexdd_nodep node)
{
    // TBD
}

