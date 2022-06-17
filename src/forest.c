
#include <stdlib.h> // malloc/free calloc realloc
#include <stdio.h>

#include "forest.h"
#include "error.h"

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

/********************************************************************
 *
 *  BDD Forest.
 *
 ********************************************************************/

void rexdd_create_forest(rexdd_forest_p F, const rexdd_forest_settings_p s)
{
    if (F == NULL) {
        rexdd_error(__FILE__, __LINE__, "Null forest");
    }
    
    // realloc the memory of the forest
    F = realloc(F,sizeof(rexdd_forest_t)); 
    
    // Initialize its members
    F->S = *s;
    if (rexdd_create_nodeman(&(F->M)) != 0) 
        rexdd_error(__FILE__, __LINE__, "Initialize the node manager error!");
    if (rexdd_create_UT(&(F->UT), &(F->M)) != 0) 
        rexdd_error(__FILE__, __LINE__, "Initialize the unique table error!");

    // TBD *roots...
    rexdd_init_function(F->roots);

}

/* ================================================================= */

void rexdd_destroy_forest(rexdd_forest_p F)
{

    rexdd_default_forest_settings(0, &(F->S));
    rexdd_destroy_nodeman(&(F->M));
    rexdd_destroy_UT(&(F->UT));
    
    // remove rexdd_function_s TBD
    rexdd_done_function(F->roots);

    if (F != NULL){
        free(F);
        F = NULL;
    }
}


/* ================================================================= */

void rexdd_reduce_edge(
        rexdd_forest_p          F,      // the forest (S, M, UT)
        uint_fast32_t           n,      // node level
        rexdd_edge_label_t      l,      // rexdd_rule_t, bool, bool
        rexdd_unpacked_node_t   p,      // uint_fast32_t level, rexdd_edge_t edge[2]
        rexdd_edge_t            *out)   // rexdd_edge_lable_t, uint_fast64_t
{
    // TBD
    if (n != p.level) {
        rexdd_error(__FILE__, __LINE__, "Target node level unmatched")
    }

    rexdd_nodeman_t M = F->M;
    rexdd_nodepage_t first_page = *(F->M->pages);

    rexdd_node_handle handle = rexdd_new_handle(&(F->M));
    
    if (rexdd_pack_handle(&(F->M), handle, &(p)) !=0){
        rexdd_error(__FILE__, __LINE__, "Fill in a packed node fail");
    }

    /*
     * Hash node
     */
    uint64_t H = HASH_HANDLE(&(F->M), handle); // HASH_HANDLE may use node itself instead of handle?
     /*
     * Check if it is in this forest
     */

    /* 
     * Reduce and get the legal and canonical node; if can not, be the same
     */

    
    /*
     * Create this new node and add to the unique table (forest)
     */

}

/********************************************************************
 *
 *  Top-level, easy to use function interface.
 *
 ********************************************************************/

void rexdd_reset_as_variable(
        rexdd_function_p    fn,
        uint32_t            v,
        bool                c)
{

}

void rexdd_init_as_variable(
        rexdd_function_p    fn,
        rexdd_forest_p      For,
        uint32_t            v,
        bool                c)
{

}

void rexdd_ITE(
        const rexdd_function_p f,
        const rexdd_function_p g,
        const rexdd_function_p h,
              rexdd_function_p result)
{

}

