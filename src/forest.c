
#include "forest.h"

/********************************************************************

    Front end.

********************************************************************/

/* ================================================================= */

void rexdd_default_forest_settings(unsigned L, rexdd_forest_settings_p s)
{
    if (s) {
        s->num_levels = L;

        // TBD
    }
}

/* ================================================================= */

rexdd_forest_p rexdd_create_forest(const rexdd_forest_settings_p s)
{
    rexdd_forest_p Fp;
    
    // TBD
    return 0;
}

/* ================================================================= */

void rexdd_destroy_forest(rexdd_forest_p F)
{
    // TBD
}


/* ================================================================= */

void rexdd_reduce_edge(
        rexdd_forest_p F,
        rexdd_edge_label_t l,
        rexdd_unpacked_node_t n,
        rexdd_edge_t *out)
{
    
}


