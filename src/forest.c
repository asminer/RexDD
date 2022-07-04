
#include <stdlib.h> // malloc/free calloc realloc
#include <stdio.h>

#include "forest.h"
#include "error.h"

/********************************************************************

    Front end.

********************************************************************/

/* ================================================================= */

void rexdd_default_forest_settings(uint_fast32_t L, rexdd_forest_settings_p s)
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
    reduced->label.rule = rexdd_rule_N;
    reduced->label.swapped = l.swapped;
    reduced->label.complemented = 0;

    rexdd_check_pattern(F, new_p, reduced);

    //

    // Normalize the four equivalent forms
    if (reduced->target == handle) {
        // normalize for the reduced edge
        rexdd_normalize_edge(new_p, reduced);
    } else {
        rexdd_merge_edge(l, reduced, out);
    }


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

    H = rexdd_insert_UT(&(F->UT), H);
    out = reduced;

    free(new_p);
    free(reduced);
    free(pN);
}

/* ================================================================= */

void rexdd_check_pattern(
        rexdd_forest_t          *F,
        rexdd_unpacked_node_t   *new_p,
        rexdd_edge_t            *reduced)
{
    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(&(F->M), new_p);
    // Terminal patterns
    if ((new_p->edge[0].target & (0x01ul << 49) == 0) && (new_p->edge[1].target & (0x01ul << 49) == 0)) {
        reduced->target = new_p->edge[0].target;
        // pattern (a) & (c)
        if (new_p->edge[0].label.rule == rexdd_rule_X ) {
            if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_X) {
                    reduced->label.rule = rexdd_rule_X;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else if (new_p->edge[1].label.rule == rexdd_rule_HN || new_p->edge[1].label.rule == rexdd_rule_AHN) {
                    reduced->label.rule = rexdd_rule_ELZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_LZ || new_p->edge[1].label.rule == rexdd_rule_ELZ) {
                    reduced->label.rule = rexdd_rule_ELZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_X) {
                    reduced->label.rule = rexdd_rule_X;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else if (new_p->edge[1].label.rule == rexdd_rule_HZ || new_p->edge[1].label.rule == rexdd_rule_AHZ) {
                    reduced->label.rule = rexdd_rule_ELN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_LN || new_p->edge[1].label.rule == rexdd_rule_ELN) {
                    reduced->label.rule = rexdd_rule_ELN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced->target = handle;
                }
            }

        }
        // pattern (b)
        else if (new_p->edge[0].label.rule == rexdd_rule_N && new_p->edge[1].label.rule == rexdd_rule_N) {
            if (new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[0].label.complemented == 0) {
                    reduced->label.rule = rexdd_rule_LZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->label.rule = rexdd_rule_LN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                }
            } else {
                reduced->target = handle;
            }
        }
        // pattern (d)
        else if (new_p->edge[1].label.rule == rexdd_rule_X) {
            if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[0].label.rule == rexdd_rule_LN || new_p->edge[0].label.rule == rexdd_rule_ALN) {
                    reduced->label.rule = rexdd_rule_EHZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 0 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_HZ || new_p->edge[1].label.rule == rexdd_rule_EHZ) {
                    reduced->label.rule = rexdd_rule_EHZ;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 1;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_LZ || new_p->edge[1].label.rule == rexdd_rule_ALZ) {
                    reduced->label.rule = rexdd_rule_EHN;
                    reduced->label.swapped = 0;
                    reduced->label.complemented = 0;
                } else {
                    reduced->target = handle;
                }
            } else if (new_p->edge[0].label.complemented == 1 && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                if (new_p->edge[1].label.rule == rexdd_rule_HN || new_p->edge[1].label.rule == rexdd_rule_EHN) {
                    reduced->label.rule = rexdd_rule_EHN;
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
        if (new_p->edge[0].label.rule == rexdd_rule_X) {
            if (new_p->edge[0].label.complemented == 0) {
                if (new_p->edge[1].label.rule == rexdd_rule_N) {
                    reduced->label.rule = rexdd_rule_LZ;
                } else if (new_p->edge[1].label.rule == rexdd_rule_LZ || new_p->edge[1].label.rule == rexdd_rule_ELZ) {
                    reduced->label.rule = rexdd_rule_ELZ;
                } else {
                    reduced->target = handle;
                }
            } else {
                if (new_p->edge[1].label.rule == rexdd_rule_N) {
                    reduced->label.rule = rexdd_rule_LN;
                } else if (new_p->edge[1].label.rule == rexdd_rule_LN || new_p->edge[1].label.rule == rexdd_rule_ELN) {
                    reduced->label.rule = rexdd_rule_ELN;
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
        if (new_p->edge[1].label.rule == rexdd_rule_X) {
            if (new_p->edge[1].label.complemented == 0) {
                if (new_p->edge[0].label.rule == rexdd_rule_N) {
                    reduced->label.rule = rexdd_rule_HZ;
                } else if (new_p->edge[0].label.rule == rexdd_rule_HZ || new_p->edge[0].label.rule == rexdd_rule_EHZ) {
                    reduced->label.rule = rexdd_rule_EHZ;
                } else {
                    reduced->target = handle;
                }
            } else {
                if (new_p->edge[0].label.rule == rexdd_rule_N) {
                    reduced->label.rule = rexdd_rule_HN;
                } else if (new_p->edge[0].label.rule == rexdd_rule_HN || new_p->edge[0].label.rule == rexdd_rule_EHN) {
                    reduced->label.rule = rexdd_rule_EHN;
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
        if (new_p->edge[0].label.rule == rexdd_rule_N) {
            if (new_p->edge[1].label.rule == rexdd_rule_N || new_p->edge[1].label.rule == rexdd_rule_X) {
                reduced->label.rule = rexdd_rule_X;
            } else {
                reduced->target = handle;
            }
        } else if (new_p->edge[0].label.rule == rexdd_rule_X) {
            if (new_p->edge[1].label.rule == rexdd_rule_N || new_p->edge[1].label.rule == rexdd_rule_X) {
                reduced->label.rule = rexdd_rule_X;
            } else if (new_p->edge[1].label.rule == rexdd_rule_HZ || new_p->edge[1].label.rule == rexdd_rule_AHZ) {
                reduced->label.rule = rexdd_rule_AHZ;
            } else if (new_p->edge[1].label.rule == rexdd_rule_HN || new_p->edge[1].label.rule == rexdd_rule_AHN) {
                reduced->label.rule = rexdd_rule_AHN;
            } else {
                reduced->target = handle;
            }
        }
        // pattern (d)
        else if (new_p->edge[0].label.rule == rexdd_rule_LZ || new_p->edge[0].label.rule == rexdd_rule_ALZ) {
            if (new_p->edge[1].label.rule == rexdd_rule_X) {
                reduced->label.rule = rexdd_rule_ALZ;
            } else {
                reduced->target = handle;
            }
        } else if (new_p->edge[1].label.rule == rexdd_rule_LN || new_p->edge[1].label.rule == rexdd_rule_ALN) {
            if (new_p->edge[1].label.rule == rexdd_rule_X) {
                reduced->label.rule = rexdd_rule_ALN;
            } else {
                reduced->target = handle;
            }
        } else {
            reduced->target = handle;
        }
    }
}

/* ================================================================= */

void rexdd_merge_edge(
        rexdd_edge_label_t      l,
        rexdd_edge_t            *reduced,
        rexdd_edge_t            *out)
{

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

