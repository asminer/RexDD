
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

rexdd_forest_p rexdd_create_forest(const rexdd_forest_settings_p s)
{
    rexdd_forest_p Fp = malloc(sizeof(rexdd_forest_p));
    if (Fp == NULL) {
        fprintf(stderr, "%s\n", "malloc error!");
        exit(1);
    }
    Fp->S = *s;
    if (rexdd_create_nodeman(&(Fp->M)) != 0) 
        fprintf(stderr, "\n%s\n", "Initialize the node manager error!");
    if (rexdd_create_UT(&(Fp->UT), &(Fp->M)) != 0) 
        fprintf(stderr, "\n%s\n", "Initialize the unique table error!");

    // TBD *roots...
    rexdd_init_function(Fp->roots);

    return Fp;
}

/* ================================================================= */

void rexdd_destroy_forest(rexdd_forest_p F)
{
    // TBD
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
        rexdd_forest_p          F,
        uint_fast32_t           n,
        rexdd_edge_label_t      l, // rexdd_rule_t, bool, bool
        rexdd_unpacked_node_t   p, // uint_fast32_t level, rexdd_edge_t edge[2]
        rexdd_edge_t            *out) // rexdd_edge_lable_t, uint_fast64_t
{
    // TBD
    /*
     * Hash node
     */

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

