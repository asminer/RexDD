
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

    if (n == 0) {
        // out edge to terminal... TBD
        /*
            set the 50th bits of the children of out edge to 0 (terminal)?
        */
    }

    /*
     * Check if it is in this forest unique table. if so, return it
     */
        /* 
        * Node handle things
            Note: node handle may includes the information of storage location (which page and chunk)?
        */

        /* check if there is a handle for this unpacked node and get the handle
                Not sure if this step can be done inside rexdd_nodeman_get_handle :)
        */
    rexdd_packed_node_p pN = malloc(sizeof(rexdd_packed_node_t));
    rexdd_unpacked_to_packed(&p, pN);
    rexdd_node_handle_t handle = 0;

    bool is_handle = 0;
    for (int h=1; h<=(0x01<<48); h++) {
        is_handle = rexdd_are_packed_duplicates(pN, rexdd_get_packed_for_handle(&(F->M),h));
        if (is_handle) {
            handle = (rexdd_node_handle_t)h;
            break;
        }
    }
    if (is_handle != 1){
        handle = rexdd_nodeman_get_handle(&(F->M), &p);
    }

    free(pN);
    pN = NULL;

    /*
     * Hash node
     */
    uint_fast64_t H = rexdd_hash_handle(&(F->M), handle); 

    rexdd_packed_node_p curr_packed;
    for (curr_packed = rexdd_get_packed_for_handle(&((&(F->UT))->M), H); curr_packed; curr_packed = rexdd_get_packed_next(curr_packed) ) {
    //                                                          ^ not sure query in the unique table
        
        if (curr_packed->fourth32 & (0x01ul << 29) - 1 != n)    continue;                   // check level
        if (((curr_packed->first64 & ~((0x01ul << 50)-1)) >> 14)
                |
            (curr_packed->second64 & (0x01ul << 36) - 1) != p.edge[0].target)   continue;   // check loch target
        if (((curr_packed->second64 & ~((0x01ul << 36) - 1)) >> 14)
                |
            (curr_packed->third32 & (0x01 << 22) - 1) != p.edge[1].target)  continue;       // check hich target
        
        // Not sure if we need to check its low and high label ...TBD
        
        out->label = l;
        out->target = handle;
        return ;
    }
    

    /* ======REDUCE======= */
    /*
        {Merge rexdd_edge_label_t and reduced node}
            {reduce node by the patterns}
    */
 
    // Not sure about its children reduce

    rexdd_unpacked_node_p new_p = malloc(sizeof(rexdd_unpacked_node_t));
    *new_p = p;
    if (l.swapped == 1) {
        // swap the target node (its loch loru and hich hiru)...TBD
        rexdd_edge_t temp = new_p->edge[0];
        new_p->edge[0] = new_p->edge[1];
        new_p->edge[1] = temp;
    }

    // check if it has a terminal or nonterminal pattern
    rexdd_edge_t *reduced;
    reduced->target = new_p->edge[0].target;
    reduced->label.rule = N;
    reduced->label.swapped = 0;
    reduced->label.complemented = 0;
    // Terminal patterns
    if ((new_p->edge[0].target & (0x01ul << 49) == 0) && (new_p->edge[1].target & (0x01ul << 49) == 0)) {
        reduced->target = new_p->edge[0].target;
        // pattern (a) & (c)
        if (new_p->edge[0].label.rule == X ) {
            if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == X) {
                    reduced->label.rule = X;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else if (new_p->edge[1].label.rule == HN || new_p->edge[1].label.rule == AHN) {
                    reduced->label.rule = ELZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced = NULL;
                }
            } else if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == LZ || new_p->edge[1].label.rule == ELZ) {
                    reduced->label.rule = ELZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced = NULL;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == X) {
                    reduced->label.rule = X;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else if (new_p->edge[1].label.rule == HZ || new_p->edge[1].label.rule == AHZ) {
                    reduced->label.rule = ELN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced = NULL;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == LN || new_p->edge[1].label.rule == ELN) {
                    reduced->label.rule = ELN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced = NULL;
                }
            } 
            
        }
        // pattern (b)
        else if (new_p->edge[0].label.rule == N && new_p->edge[1].label.rule == N) {
            if (new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[0].label.complemented == 0) {
                    reduced->label.rule = LZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->label.rule = LN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                }
            } else {
                reduced = NULL;
            }
        }
        // pattern (d)
        else if (new_p->edge[1].label.rule == X) {
            if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[0].label.rule == LN || new_p->edge[0].label.rule == ALN) {
                    reduced->label.rule = EHZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced = NULL;
                }
            } else if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == HZ || new_p->edge[1].label.rule == EHZ) {
                    reduced->label.rule = EHZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced = NULL;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == LZ || new_p->edge[1].label.rule == ALZ) {
                    reduced->label.rule = EHN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced = NULL;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == HN || new_p->edge[1].label.rule == EHN) {
                    reduced->label.rule = EHN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced = NULL;
                }
            } 

        } else {
            reduced = NULL;
        }
    } 
    // Nonterminal patterns 1)
    else if ((new_p->edge[0].target & (0x01ul << 49) == 0) && (new_p->edge[1].target & (0x01ul << 49) != 0)) {
        reduced->target = new_p->edge[1].target;
        reduced->label.swapped = new_p->edge[1].label.swapped;
        reduced->label.complemented = new_p->edge[1].label.complemented;
        // pattern (b) & (c)
        if (new_p->edge[0].label.rule == X) {
            if (new_p->edge[0].label.complemented == 0) {
                if (new_p->edge[1].label.rule == N) {
                    reduced->label.rule = LZ;
                } else if (new_p->edge[1].label.rule == LZ || new_p->edge[1].label.rule == ELZ) {
                    reduced->label.rule = ELZ;
                } else {
                    reduced =NULL;
                }
            } else {
                if (new_p->edge[1].label.rule == N) {
                    reduced->label.rule = LN;
                } else if (new_p->edge[1].label.rule == LN || new_p->edge[1].label.rule == ELN) {
                    reduced->label.rule = ELN;
                } else {
                    reduced =NULL;
                }
            }
        } else {
            reduced = NULL;
        }
    } 
    // Nonterminal parttern 2)
    else if ((new_p->edge[0].target & (0x01ul << 49) != 0) && (new_p->edge[1].target & (0x01ul << 49) == 0)) {
        reduced->target = new_p->edge[0].target;
        reduced->label.swapped = new_p->edge[0].label.swapped;
        reduced->label.complemented = new_p->edge[0].label.complemented;
        // pattern (e) & (f)
        if (new_p->edge[1].label.rule == X) {
            if (new_p->edge[1].label.complemented == 0) {
                if (new_p->edge[0].label.rule == N) {
                    reduced->label.rule = HZ;
                } else if (new_p->edge[0].label.rule == HZ || new_p->edge[0].label.rule == EHZ) {
                    reduced->label.rule = EHZ;
                } else {
                    reduced =NULL;
                }
            } else {
                if (new_p->edge[0].label.rule == N) {
                    reduced->label.rule = HN;
                } else if (new_p->edge[0].label.rule == HN || new_p->edge[0].label.rule == EHN) {
                    reduced->label.rule = EHN;
                } else {
                    reduced =NULL;
                }
            }
        } else {
            reduced = NULL;
        }
    }
    // Nonterminal pattern 3)
    else if ((new_p->edge[0].target & (0x01ul << 49) != 0) && (new_p->edge[1].target == new_p->edge[0].target)) {
        //                                                                            ^ maybe the lower 50 bits?
        reduced->target = new_p->edge[0].target;
        reduced->label.swapped = new_p->edge[0].label.swapped;
        reduced->label.complemented = new_p->edge[0].label.complemented;
        // pattern (a) & (g)
        if (new_p->edge[0].label.rule == N) {
            if (new_p->edge[1].label.rule == N || new_p->edge[1].label.rule == X) {
                reduced->label.rule = X;
            } else {
                reduced = NULL;
            }
        } else if (new_p->edge[0].label.rule == X) {
            if (new_p->edge[1].label.rule == N || new_p->edge[1].label.rule == X) {
                reduced->label.rule = X;
            } else if (new_p->edge[1].label.rule == HZ || new_p->edge[1].label.rule == AHZ) {
                reduced->label.rule = AHZ;
            } else if (new_p->edge[1].label.rule == HN || new_p->edge[1].label.rule == AHN) {
                reduced->label.rule = AHN;
            } else {
                reduced = NULL;
            }
        }
        // pattern (d)
        else if (new_p->edge[0].label.rule == LZ || new_p->edge[0].label.rule == ALZ) {
            if (new_p->edge[1].label.rule == X) {
                reduced->label.rule = ALZ;
            } else {
                reduced = NULL;
            }
        } else if (new_p->edge[1].label.rule == LN || new_p->edge[1].label.rule == ALN) {
            if (new_p->edge[1].label.rule == X) {
                reduced->label.rule = ALN;
            } else {
                reduced = NULL;
            }
        } else {
            reduced = NULL;
        }
    }
    // Normalize the four equivalent forms
    else {
        // TBD
    }

    // Normalize out (figure 6: the decision table to choose canonical node)

    // Check if l and reduced can be merged
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

