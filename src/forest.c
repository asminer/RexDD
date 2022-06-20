
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
    rexdd_init_nodeman(&(F->M),0);
    
    if (rexdd_create_UT(&(F->UT), &(F->M)) != 0) 
        rexdd_error(__FILE__, __LINE__, "Initialize the unique table error!");

    // TBD *roots...
    rexdd_init_function(F->roots);

}

/* ================================================================= */

void rexdd_destroy_forest(rexdd_forest_p F)
{

    rexdd_default_forest_settings(0, &(F->S));
    rexdd_free_nodeman(&(F->M));
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
    if (n != p.level) {
        rexdd_error(__FILE__, __LINE__, "Target node level unmatched");
    }


    /* 
     * Node handle things [confused] TBD
        Note: node handle may includes the information of storage location (which page and chunk)?
     */

    rexdd_node_handle handle = rexdd_new_handle(&(F->M));
    if (rexdd_pack_handle(&(F->M), handle, &(p)) !=0){
        rexdd_error(__FILE__, __LINE__, "Fill in a packed node fail");
    }


    /*
     * Hash node
     */
    uint64_t H = HASH_HANDLE(&(F->M), handle); // HASH_HANDLE may use node itself instead of handle?
    /*
     * Check if it is in this forest. if so, return it; or do the following steps
     */
    // TBD

    /* ======REDUCE======= */
    /*
        {Merge rexdd_edge_label_t and reduced node}
            {reduce node by the patterns}
    */

    uint_fast64_t terminal;     // TBD the terminal node handle?
    if (p.level == 0){
        out->target = terminal;
        out->label = l;
    }
 
    rexdd_packed_node_p pN;
    rexdd_unpacked_node_p uN = &p;
    rexdd_unpacked_to_packed(uN, pN);
    // 
    if (l.swapped == 1) {
        // swap the target node (its loch loru and hich hiru)TBD

    }

    rexdd_edge_t *reduced;
    if ((pN->first64 & 0xf000000000000000 == 1)|| (pN->second64 & 0xf000000000000000 == 1)) {
        // loch or hich are nonterm
        // pattern (a)
        if ((((pN->third32 & LORU_MASK) >> 22 == N)||((pN->third32 & LORU_MASK) >> 22 == X))
                && (((pN->third32 & HIRU_MASK) >> 27 == N)||((pN->third32 & HIRU_MASK) >> 27 == X))){
            reduced->target =   ((pN->first64 & TOP14_MASK) >> 14)
                            |
                            (pN->second64 & LOW36_MASK);
            // ... TBD

        }
        // pattern (b)-(g) TBD
    } else {
        // loch and hich are term
        // pattern (a)-(g) TBD
    }

    // Normalize out (figure 6: the decision table to choose canonical node)

    // Merge l and reduced into out TBD

    /* ======END REDUCE====== */
    
    /*
     * Create this new node and add to the unique table (forest)
     */
    // TBD

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

