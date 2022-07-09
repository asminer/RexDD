
#include <stdlib.h> // malloc/free calloc realloc
#include <stdio.h>

#include "forest.h"
#include "error.h"

/********************************************************************

    Front end.

********************************************************************/

/* ================================================================= */

void rexdd_default_forest_settings(unsigned L, rexdd_forest_settings_t *s)
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

void rexdd_init_forest(rexdd_forest_t *F, const rexdd_forest_settings_t *s)
{
    rexdd_sanity1(F, "null forest in rexdd_init_forest");

    // Initialize its members
    F->S = *s;
    F->M = malloc(sizeof(rexdd_nodeman_t));
    rexdd_init_nodeman(F->M,0);
    F->UT = malloc(sizeof(rexdd_unique_table_t));
    rexdd_init_UT(F->UT, F->M);
    F->roots = malloc(sizeof(rexdd_function_t));
    rexdd_init_function(F->roots);

}

/* ================================================================= */

void rexdd_free_forest(rexdd_forest_t *F)
{

    rexdd_default_forest_settings(0, &(F->S));
    rexdd_free_UT(F->UT);
    rexdd_free_nodeman(F->M);

    // remove rexdd_function_s TBD
    rexdd_done_function(F->roots);
}


/* ================================================================= */

void rexdd_normalize_edge(
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *out)
{
    if (rexdd_is_terminal(P->edge[0].target) == 0)  P->edge[0].label.swapped = 0;
    if (rexdd_is_terminal(P->edge[1].target) == 0)  P->edge[1].label.swapped = 0;
    
    if (P->edge[0].target < P->edge[1].target) {
        // (a, s_a) < (b, s_b)
        if (P->edge[0].label.complemented == 1){
            out->label.complemented = !out->label.complemented;
            rexdd_edge_com (P->edge[0]);
            rexdd_edge_com (P->edge[1]);
        } else {

        }
    } else if (P->edge[0].target == P->edge[1].target) {
        if (P->edge[0].label.swapped < P->edge[1].label.swapped) {
            // (a, s_a) < (b, s_b)
            if (P->edge[0].label.complemented == 1){
                out->label.complemented = !out->label.complemented;
                rexdd_edge_com (P->edge[0]);
                rexdd_edge_com (P->edge[1]);
            } else {

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
                if (P->edge[0].label.rule > P->edge[1].label.rule) {
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
                if (P->edge[0].label.rule > rexdd_rule_com_t(P->edge[1].label.rule)) {
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

void rexdd_check_pattern(
        rexdd_forest_t          *F,
        rexdd_unpacked_node_t   *new_p,
        rexdd_edge_t            *reduced)
{
    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);
    // Terminal patterns
    if ((rexdd_is_terminal(new_p->edge[0].target) == 0) && (rexdd_is_terminal(new_p->edge[1].target) == 0)) {
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
    else if ((rexdd_is_terminal(new_p->edge[0].target) == 0) && (rexdd_is_terminal(new_p->edge[1].target) != 0)) {
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
    else if ((rexdd_is_terminal(new_p->edge[0].target) != 0) && (rexdd_is_terminal(new_p->edge[1].target) == 0)) {
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
                    reduced->label.rule =  rexdd_rule_EHN;
                } else {
                    reduced->target = handle;
                }
            }
        } else {
            reduced->target = handle;
        }
    }
    // Nonterminal pattern 3)
    else if ((rexdd_is_terminal(new_p->edge[0].target) != 0) 
                && new_p->edge[1].target == new_p->edge[0].target) {
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
    rexdd_forest_t              *F,
        uint32_t                n,
        rexdd_edge_label_t      l,
        rexdd_edge_t            *reduced,
        rexdd_edge_t            *out)
{
    if (l.complemented == 1) rexdd_edge_com (*reduced);

    if (reduced->label.rule == rexdd_rule_N) {
        reduced->label.rule = l.rule;
        out = reduced;
    }
    if ((2 <= l.rule && l.rule <= 13)
        && reduced->label.rule == rexdd_rule_X
        && l.rule%2 == reduced->label.complemented
        && rexdd_is_terminal(reduced->target) == 0) {
        out = reduced;
    }
    if ((l.rule == rexdd_rule_HZ || l.rule == rexdd_rule_HN || l.rule == rexdd_rule_EHZ || l.rule == rexdd_rule_EHN)
        && l.rule%2 == reduced->label.complemented
        && (reduced->label.rule == rexdd_rule_LZ || reduced->label.rule == rexdd_rule_LN)
        && reduced->label.rule%2 != reduced->label.complemented
        && rexdd_is_terminal(reduced->target) == 0) {
        out = reduced;
        if (reduced->label.complemented == 1) {
            out->label.rule = rexdd_rule_EHN;
            out->label.complemented = 0;
        } else {
            out->label.rule = rexdd_rule_EHZ;
            out->label.complemented = 1;
        }
    }
    if (l.rule == rexdd_rule_N
        ||
        (l.rule == rexdd_rule_X && reduced->label.rule == rexdd_rule_N)) {
        out = reduced;
    } else if ((l.rule == rexdd_rule_LZ || l.rule == rexdd_rule_LN || l.rule == rexdd_rule_ELZ || l.rule == rexdd_rule_ELN)
             && (reduced->label.rule == rexdd_rule_LZ || reduced->label.rule == rexdd_rule_LN
                || reduced->label.rule == rexdd_rule_ELZ || reduced->label.rule == rexdd_rule_ELN)
             && l.rule%2 == reduced->label.rule%2) {
        out = reduced;
        if (l.rule%2 == 0) {
            out->label.rule = rexdd_rule_ELZ;
        } else {
            out->label.rule = rexdd_rule_ELN;
        }
    } else if ((l.rule == rexdd_rule_HZ || l.rule == rexdd_rule_HN || l.rule == rexdd_rule_EHZ || l.rule == rexdd_rule_EHN)
             && (reduced->label.rule == rexdd_rule_HZ || reduced->label.rule == rexdd_rule_HN
                || reduced->label.rule == rexdd_rule_EHZ || reduced->label.rule == rexdd_rule_EHN)
             && l.rule%2 == reduced->label.rule%2) {
        out = reduced;
        if (l.rule%2 == 0) {
            out->label.rule = rexdd_rule_EHZ;
        } else {
            out->label.rule = rexdd_rule_EHN;
        }
    } else if (1 <= l.rule && l.rule <=9) {
        // push up one
        rexdd_unpacked_node_t *new_p = malloc(sizeof(rexdd_unpacked_node_t));
        new_p->level = n+1;
        out->label.complemented = 0;
        out->label.swapped = 0;

        if (l.rule == rexdd_rule_X) {
            new_p->edge[0] = *reduced;
            new_p->edge[1] = *reduced;
            out->label.rule = rexdd_rule_N;

        } else if (l.rule == rexdd_rule_LZ || l.rule == rexdd_rule_LN || l.rule == rexdd_rule_ELZ || l.rule == rexdd_rule_ELN) {

            new_p->edge[0].label.rule = rexdd_rule_X;
            new_p->edge[0].label.swapped = 0;
            new_p->edge[0].target = rexdd_make_terminal(reduced->target);
            new_p->edge[1] = *reduced;

            if (l.rule%2 == 0) {
                new_p->edge[0].label.complemented = 0;
                out->label.rule = rexdd_rule_LZ;
            } else {
                new_p->edge[0].label.complemented = 1;
                out->label.rule = rexdd_rule_LN;
            }
        }else if (l.rule == rexdd_rule_HZ || l.rule == rexdd_rule_HN || l.rule == rexdd_rule_EHZ || l.rule == rexdd_rule_EHN) {

            new_p->edge[1].label.rule = rexdd_rule_X;
            new_p->edge[1].label.swapped = 0;
            new_p->edge[1].target = rexdd_make_terminal(reduced->target);
            new_p->edge[0] = *reduced;
            if (l.rule%2 == 0) {
                new_p->edge[1].label.complemented = 0;
                out->label.rule = rexdd_rule_HZ;
            } else {
                new_p->edge[1].label.complemented = 1;
                out->label.rule = rexdd_rule_HN;
            }

        }
        rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);
        out->target = handle;
        rexdd_normalize_edge (new_p, out);

    } else {
        // push up all TBD
        out = reduced;
    }
}

/* ================================================================= */

void rexdd_reduce_edge(
        rexdd_forest_t          *F,      // the forest (S, M, UT)
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

    // rexdd_unpacked_node_t *loch = malloc(sizeof(rexdd_unpacked_node_t));
    // rexdd_unpacked_node_t *hich = malloc(sizeof(rexdd_unpacked_node_t));
    // rexdd_unpack_handle (&(F->M), new_p->edge[0].target, loch);
    // rexdd_unpack_handle (&(F->M), new_p->edge[1].target, hich);
    // rexdd_reduce_edge(F, n-1, new_p->edge[0].label, *loch, &(new_p->edge[0]));
    // rexdd_reduce_edge(F, n-1, new_p->edge[1].label, *hich, &(new_p->edge[1]));
    // free(loch);
    // free(hich);

    if (l.swapped == 1) {
        rexdd_node_sw (new_p);
    }

    // check if it has a terminal or nonterminal pattern
    rexdd_edge_t *reduced = malloc(sizeof(rexdd_edge_t));
    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);
    reduced->target = handle;
    reduced->label.rule = rexdd_rule_N;
    reduced->label.swapped = l.swapped;
    reduced->label.complemented = l.complemented;

    // avoid push up all for the AL or AH cases in merge part
    if (l.rule <= 9 ) {
        rexdd_check_pattern(F, new_p, reduced);
    }

    //

    // Normalize the four equivalent forms
    if (reduced->label.rule == rexdd_rule_N) {
        // normalize for the reduced edge if it has no forbidden patterns
        rexdd_normalize_edge(new_p, reduced);

        /*
        * Check if it is in this forest unique table. if so, return it
        */

        uint_fast64_t H = rexdd_insert_UT(F->UT, handle);
        out = reduced;
        out->label.rule = l.rule;
        out->target = H;

    } else {
        rexdd_merge_edge(F, n, l, reduced, out);
        if ((rexdd_get_packed_for_handle(F->M, out->target)->fourth32 & ((0x01ul << 29) - 1)) > n) {
            rexdd_unpacked_node_t *temp = malloc(sizeof(rexdd_unique_table_t));
            rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, out->target) ,temp);
            rexdd_reduce_edge (F, n+1, l, *temp, out);
            free(temp);
        }

        uint_fast64_t H = rexdd_insert_UT(F->UT, out->target);
        out->target = H;
    }

    free(new_p);
    free(reduced);

}

/********************************************************************
 *
 *  Top-level, easy to use function interface.
 *
 ********************************************************************/

/*=======================
|| We ignore for now   ||
=======================*/

// void rexdd_reset_as_variable(
//         rexdd_function_t    *fn,
//         uint32_t            v,
//         bool                c)
// {

// }

// void rexdd_init_as_variable(
//         rexdd_function_t    *fn,
//         rexdd_forest_t      *For,
//         uint32_t            v,
//         bool                c)
// {

// }

// void rexdd_ITE(
//         const rexdd_function_p f,
//         const rexdd_function_p g,
//         const rexdd_function_p h,
//               rexdd_function_p result)
// {

// }

