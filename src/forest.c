
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
    
    rexdd_create_UT(&(F->UT), &(F->M));
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

void rexdd_normalize_edge(
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *out)
{
    if (P->edge[0].target < P->edge[1].target) {
        // (a, s_a) < (b, s_b)
        if (P->edge[0].label.complemented == 1){
            out->label.complemented = !out->label.complemented;
            rexdd_edge_com (P->edge[0]);
            rexdd_edge_com (P->edge[1]);
        } else {
            out = out;
        }
    } else if (P->edge[0].target == P->edge[1].target) {
        if (P->edge[0].label.swapped < P->edge[1].label.swapped) {
            // (a, s_a) < (b, s_b)
            if (P->edge[0].label.complemented == 1){
                out->label.complemented = !out->label.complemented;
                rexdd_edge_com (P->edge[0]);
                rexdd_edge_com (P->edge[1]);
            } else {
                out = out;
            }
        } else if (P->edge[0].label.swapped > P->edge[1].label.swapped) {
            // (a, s_a) > (b, s_b)
            if (P->edge[1].label.complemented == 1){
                out->label.complemented = !out->label.complemented;
                out->label.swapped = !out->label.swapped;
                rexdd_edge_com (P->edge[0]);
                rexdd_edge_com (P->edge[1]);
                rexdd_node_sw (P);
            } else {
                out->label.swapped = !out->label.swapped;
                rexdd_node_sw (P);
            }
        } else {
            // (a, s_a) = (b, s_b)
            if (P->edge[0].label.complemented == 0 && P->edge[1].label.complemented == 0) {
                if (P->edge[0].label.rule <= P->edge[1].label.rule) {
                    out = out;
                } else {
                    out->label.swapped = !out->label.swapped;
                    rexdd_node_sw (P);
                }
            } else if (P->edge[0].label.complemented == 1 && P->edge[1].label.complemented == 1) {
                if (rexdd_rule_com_t(P->edge[0].label.rule) <= rexdd_rule_com_t(P->edge[1].label.rule)) {
                    out->label.complemented = !out->label.complemented;
                    rexdd_edge_com (P->edge[0]);
                    rexdd_edge_com (P->edge[1]);
                } else {
                    out->label.complemented = !out->label.complemented;
                    out->label.swapped = !out->label.swapped;
                    rexdd_edge_com (P->edge[0]);
                    rexdd_edge_com (P->edge[1]);
                    rexdd_node_sw (P);
                }
            } else if (P->edge[0].label.complemented == 0 && P->edge[1].label.complemented == 1) {
                if (P->edge[0].label.rule <= rexdd_rule_com_t(P->edge[1].label.rule)) {
                    out = out;
                } else {
                    out->label.complemented = !out->label.complemented;
                    out->label.swapped = !out->label.swapped;
                    rexdd_edge_com (P->edge[0]);
                    rexdd_edge_com (P->edge[1]);
                    rexdd_node_sw (P);
                }
            } else {
                if (rexdd_rule_com_t(P->edge[0].label.rule) <= P->edge[1].label.rule) {
                    out->label.complemented = !out->label.complemented;
                    rexdd_edge_com (P->edge[0]);
                    rexdd_edge_com (P->edge[1]);
                } else {
                    out->label.swapped = !out->label.swapped;
                    rexdd_node_sw (P);
                }
            }
        }

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

    /* ======REDUCE======= */
 
    // new node at the same level
    rexdd_unpacked_node_t *new_p = malloc(sizeof(rexdd_unpacked_node_t));
    *new_p = p;
    
    new_p->edge[0];
    rexdd_unpacked_node_t *loch, *hich;
    rexdd_unpack_handle (&(F->M), new_p->edge[0].target, loch);
    rexdd_unpack_handle (&(F->M), new_p->edge[1].target, hich);
    rexdd_reduce_edge(F, n-1, new_p->edge[0].label, *loch, &(new_p->edge[0]));
    rexdd_reduce_edge(F, n-1, new_p->edge[1].label, *hich, &(new_p->edge[1]));

    if (l.swapped == 1) {
        rexdd_node_sw (new_p);
    }

    // check if it has a terminal or nonterminal pattern
    rexdd_edge_t *reduced = malloc(sizeof(rexdd_edge_t));
    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(&(F->M), new_p);
    reduced->target = handle;
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
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == LZ || new_p->edge[1].label.rule == ELZ) {
                    reduced->label.rule = ELZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->target = handle;
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
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == LN || new_p->edge[1].label.rule == ELN) {
                    reduced->label.rule = ELN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced->target = handle;
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
                reduced->target = handle;
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
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == HZ || new_p->edge[1].label.rule == EHZ) {
                    reduced->label.rule = EHZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == LZ || new_p->edge[1].label.rule == ALZ) {
                    reduced->label.rule = EHN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == HN || new_p->edge[1].label.rule == EHN) {
                    reduced->label.rule = EHN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced->target = handle;
                }
            } 

        } else {
            reduced->target = handle;
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
                    reduced->target = handle;
                }
            } else {
                if (new_p->edge[1].label.rule == N) {
                    reduced->label.rule = LN;
                } else if (new_p->edge[1].label.rule == LN || new_p->edge[1].label.rule == ELN) {
                    reduced->label.rule = ELN;
                } else {
                    reduced->target = handle;
                }
            }
        } else {
            reduced->target = handle;
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
                    reduced->target = handle;
                }
            } else {
                if (new_p->edge[0].label.rule == N) {
                    reduced->label.rule = HN;
                } else if (new_p->edge[0].label.rule == HN || new_p->edge[0].label.rule == EHN) {
                    reduced->label.rule = EHN;
                } else {
                    reduced->target = handle;
                }
            }
        } else {
            reduced->target = handle;
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
                reduced->target = handle;
            }
        } else if (new_p->edge[0].label.rule == X) {
            if (new_p->edge[1].label.rule == N || new_p->edge[1].label.rule == X) {
                reduced->label.rule = X;
            } else if (new_p->edge[1].label.rule == HZ || new_p->edge[1].label.rule == AHZ) {
                reduced->label.rule = AHZ;
            } else if (new_p->edge[1].label.rule == HN || new_p->edge[1].label.rule == AHN) {
                reduced->label.rule = AHN;
            } else {
                reduced->target = handle;
            }
        }
        // pattern (d)
        else if (new_p->edge[0].label.rule == LZ || new_p->edge[0].label.rule == ALZ) {
            if (new_p->edge[1].label.rule == X) {
                reduced->label.rule = ALZ;
            } else {
                reduced->target = handle;
            }
        } else if (new_p->edge[1].label.rule == LN || new_p->edge[1].label.rule == ALN) {
            if (new_p->edge[1].label.rule == X) {
                reduced->label.rule = ALN;
            } else {
                reduced->target = handle;
            }
        } else {
            reduced->target = handle;
        }
    }

    // 

    // Normalize the four equivalent forms
    if (reduced->target == handle) {
        // normalize for the reduced edge
        rexdd_normalize_edge(new_p, reduced);
    }

    // Normalize out (figure 6: the decision table to choose canonical node)

    // Check if l and reduced can be merged
        // Merge l and reduced into out TBD

    /* ======END REDUCE====== */


    /* ======MERGE====== */

    /*
        {Merge rexdd_edge_label_t and reduced node}
    */

    /* ======END MERGE====== */

    /*
     * Check if it is in this forest unique table. if so, return it
     */
    rexdd_packed_node_t *pN = malloc(sizeof(rexdd_packed_node_t));
    rexdd_unpacked_to_packed(new_p, pN);

    /*
     * Hash node
     */
    uint_fast64_t m;
    uint_fast64_t H = rexdd_hash_packed(pN, m);
    //                                      ^ Here number m TBD

    rexdd_packed_node_t *curr_packed;
    for (curr_packed = rexdd_get_packed_for_handle(&(F->UT.M), H); curr_packed; curr_packed = rexdd_get_packed_next(curr_packed) ) {
        
        if (rexdd_are_packed_duplicates(pN, curr_packed) != 1)  continue;
        else {
            out = reduced;
            return ;
        }
        
    }

    H = rexdd_insert_UT(&(F->UT.M), H);
    out = reduced;
    

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

