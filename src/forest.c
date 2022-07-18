
#include <stdlib.h> // malloc/free calloc realloc
#include <stdio.h>

#include "forest.h"
#include "error.h"

/********************************************************************

    Front end.

********************************************************************/

/* ================================================================= */

void rexdd_default_forest_settings(uint_fast32_t L, rexdd_forest_settings_t *s)
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
    if (rexdd_is_terminal(P->edge[0].target) == 1)  P->edge[0].label.swapped = 0;
    if (rexdd_is_terminal(P->edge[1].target) == 1)  P->edge[1].label.swapped = 0;

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
        rexdd_node_handle_t     handle,
        rexdd_unpacked_node_t   *new_p,
        rexdd_edge_t            *reduced)
{
    uint_fast32_t ln = new_p->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, new_p->edge[0].target));
    uint_fast32_t hn = new_p->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, new_p->edge[1].target));
    
    // Terminal patterns
    if ((rexdd_is_terminal(new_p->edge[0].target) == 1) && (rexdd_is_terminal(new_p->edge[1].target) == 1)) {
        if (new_p->edge[0].target <= new_p->edge[0].target) {
            reduced->target = new_p->edge[0].target;
        } else {
            reduced->target = new_p->edge[1].target;
        }

        rexdd_sanity1(ln == hn, "low edge and high edge skip level error");
        
        // pattern (a)
        if (new_p->edge[0].label.rule == rexdd_rule_X 
            && ln > 1
            && new_p->edge[0].label.rule == new_p->edge[1].label.rule
            && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
            
            reduced->label.rule = rexdd_rule_X;
            reduced->label.complemented = new_p->edge[0].label.complemented;

        // pattern (b)
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln == 1
                    && new_p->edge[0].label.rule == new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {

                        reduced->label.rule = rexdd_rule_X;
                        reduced->label.complemented = new_p->edge[0].label.complemented;

        // pattern (c)
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln == 1
                    && new_p->edge[0].label.rule == new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {

                    if (new_p->edge[0].label.complemented == 0) {
                        reduced->label.rule = rexdd_rule_EL0;
                    } else {
                        reduced->label.rule = rexdd_rule_EL1;
                    }
                    reduced->label.complemented = new_p->edge[1].label.complemented;

        // pattern (d)
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln>1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {

                    if (new_p->edge[0].label.complemented == 0
                        && new_p->edge[1].label.rule == rexdd_rule_EL0) {
                        reduced->label.rule = rexdd_rule_EL0;
                    } else if (new_p->edge[0].label.complemented == 1
                        && new_p->edge[1].label.rule == rexdd_rule_EL1){
                        reduced->label.rule = rexdd_rule_EL1;
                    }
                    reduced->label.complemented = new_p->edge[1].label.complemented;

        // pattern (e)
        } else if (new_p->edge[1].label.rule == rexdd_rule_X
                    && hn>1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {

                    if (new_p->edge[1].label.complemented == 0
                        && new_p->edge[0].label.rule == rexdd_rule_AL0) {
                        reduced->label.rule = rexdd_rule_EH1;
                    } else if (new_p->edge[1].label.complemented == 1
                        && new_p->edge[0].label.rule == rexdd_rule_AL1){
                        reduced->label.rule = rexdd_rule_EH0;
                    }
                    reduced->label.complemented = !new_p->edge[0].label.complemented;

        // pattern (f)
        } else if (new_p->edge[1].label.rule == rexdd_rule_X
                    && hn>1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {

                    if (new_p->edge[1].label.complemented == 0
                        && new_p->edge[0].label.rule == rexdd_rule_EH0) {
                        reduced->label.rule = rexdd_rule_EH0;
                    } else if (new_p->edge[1].label.complemented == 1
                        && new_p->edge[0].label.rule == rexdd_rule_EH1){
                        reduced->label.rule = rexdd_rule_EH1;
                    }
                    reduced->label.complemented = new_p->edge[0].label.complemented;

        // pattern (g)
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln>1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                    
                    if (new_p->edge[0].label.complemented == 0
                        && new_p->edge[1].label.rule == rexdd_rule_AH0) {
                        reduced->label.rule = rexdd_rule_EL1;
                    } else if (new_p->edge[0].label.complemented == 1
                        && new_p->edge[1].label.rule == rexdd_rule_AH1){
                        reduced->label.rule = rexdd_rule_EL0;
                    }
                    reduced->label.complemented = !new_p->edge[1].label.complemented;
        // t = c :
        //      (d) & (g)
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln > 1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {

                        if (new_p->edge[0].label.complemented == 0
                            && (new_p->edge[1].label.rule == rexdd_rule_EL0
                                || new_p->edge[1].label.rule == rexdd_rule_AH0)) {
                                    reduced->label.rule = rexdd_rule_X;
                        } else if (new_p->edge[0].label.complemented == 1
                            && (new_p->edge[1].label.rule == rexdd_rule_EL1
                                ||new_p->edge[1].label.rule == rexdd_rule_AH1)) {
                                    reduced->label.rule = rexdd_rule_X;
                        } else {
                            reduced->target = handle;
                        }
                        reduced->label.complemented = new_p->edge[0].label.complemented;
        //      (f) & (e)
        } else if (new_p->edge[1].label.rule == rexdd_rule_X
                    && hn > 1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {

                        if (new_p->edge[1].label.complemented == 0
                            && (new_p->edge[0].label.rule == rexdd_rule_AL0
                                ||new_p->edge[0].label.rule == rexdd_rule_EH0)) {
                                    reduced->label.rule = rexdd_rule_X;
                        } else if (new_p->edge[1].label.complemented == 1
                            && (new_p->edge[0].label.rule == rexdd_rule_AL1
                                ||new_p->edge[0].label.rule == rexdd_rule_EH1)) {
                                    reduced->label.rule = rexdd_rule_X;
                        } else {
                            reduced->target = handle;
                        }
                        reduced->label.complemented = new_p->edge[0].label.complemented;
        }else {
            reduced->target = handle;
        }
    }
    // Nonterminal patterns 1)
    else if ((rexdd_is_terminal(new_p->edge[0].target) == 1) && (rexdd_is_terminal(new_p->edge[1].target) == 0)) {
        reduced->target = new_p->edge[1].target;
        reduced->label.swapped = new_p->edge[1].label.swapped;
        reduced->label.complemented = new_p->edge[1].label.complemented;
        
        rexdd_sanity1(ln != hn, "low edge and high edge skip level error");

        // pattern (b) & (c)
        if (new_p->edge[0].label.rule == rexdd_rule_X
            && ln > 1
            && ln > hn
            ) {
            if (new_p->edge[0].label.complemented == 0) {
                if (new_p->edge[1].label.rule == rexdd_rule_X && (hn == 1)
                    || new_p->edge[1].label.rule == rexdd_rule_EL0) {
                    reduced->label.rule = rexdd_rule_EL0;
                } else {
                    reduced->target = handle;
                }
            } else {
                if ((new_p->edge[1].label.rule == rexdd_rule_X && (hn == 1))
                    || new_p->edge[1].label.rule == rexdd_rule_EL1) {
                        reduced->label.rule = rexdd_rule_EL1;
                } else {
                    reduced->target = handle;
                }
            }
        } else {
            reduced->target = handle;
        }
    }
    // Nonterminal parttern 2)
    else if ((rexdd_is_terminal(new_p->edge[0].target) == 0) && (rexdd_is_terminal(new_p->edge[1].target) == 1)) {
        reduced->target = new_p->edge[0].target;
        reduced->label.swapped = new_p->edge[0].label.swapped;
        reduced->label.complemented = new_p->edge[0].label.complemented;

        rexdd_sanity1(ln != hn, "low edge and high edge skip level error");

        // pattern (e) & (f)
        if (new_p->edge[1].label.rule == rexdd_rule_X
            && hn > 1
            && hn > ln
            ) {
            if (new_p->edge[1].label.complemented == 0) {
                if ((new_p->edge[0].label.rule == rexdd_rule_X && (ln == 1))
                    || new_p->edge[0].label.rule == rexdd_rule_EH0) {
                    reduced->label.rule = rexdd_rule_EH0;
                } else {
                    reduced->target = handle;
                }
            } else {
                if ((new_p->edge[0].label.rule == rexdd_rule_X && (ln == 1))
                    || new_p->edge[0].label.rule == rexdd_rule_EH1) {
                    reduced->label.rule = rexdd_rule_EH1;
                } else {
                    reduced->target = handle;
                }
            }
        } else {
            reduced->target = handle;
        }
    }
    // Nonterminal pattern 3)
    else if ((rexdd_is_terminal(new_p->edge[0].target) == 0)
                && new_p->edge[1].target == new_p->edge[0].target) {
        reduced->target = new_p->edge[0].target;
        reduced->label.swapped = new_p->edge[0].label.swapped;
        reduced->label.complemented = new_p->edge[0].label.complemented;

        rexdd_sanity1(ln == hn, "low edge and high edge skip level error");

        // pattern (a)
        if (new_p->edge[0].label.rule == rexdd_rule_X 
            && new_p->edge[1].label.rule == rexdd_rule_X
            ) {
            reduced->label.rule = rexdd_rule_X;
        // pattern (g)
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
            && ln > 1
            && ((rexdd_is_EH(new_p->edge[1].label.rule) && (hn == 2))
                || (rexdd_is_AH(new_p->edge[1].label.rule) && (hn>=3)))
            ) {
            if (rexdd_is_one(new_p->edge[1].label.rule)) {
                reduced->label.rule = rexdd_rule_AH1;
            } else {
                reduced->label.rule = rexdd_rule_AH0;
            }
        // pattern (d)
        } else if (new_p->edge[1].label.rule == rexdd_rule_X
            && hn > 1
            && ((rexdd_is_EL(new_p->edge[1].label.rule) && (hn == 2))
                || (rexdd_is_AL(new_p->edge[1].label.rule) && (hn>=3)))
            ) {
            if (rexdd_is_one(new_p->edge[1].label.rule)) {
                reduced->label.rule = rexdd_rule_AL1;
            } else {
                reduced->label.rule = rexdd_rule_AL0;
            }
        } else {
            reduced->target = handle;
        }
    }
}

/* ================================================================= */

void rexdd_merge_edge(
        rexdd_forest_t          *F,
        uint32_t                m,
        uint32_t                n,
        rexdd_edge_label_t      l,
        rexdd_edge_t            *reduced,
        rexdd_edge_t            *out)
{
    uint32_t incoming_skip = m - n;
    uint32_t reduced_skip = n - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, reduced->target));

    if (l.complemented == 1) rexdd_edge_com (*reduced);

    // Unreduceable and compatible merge
    if (reduced->label.rule == rexdd_rule_X && (reduced_skip==0)) {
            reduced->label.rule = l.rule;
            out = reduced;
    }

    // compatible merge with two long edges when considering terminal node (a)
    if (reduced->label.rule == rexdd_rule_X
        && l.rule != rexdd_rule_X
        && rexdd_is_one(l.rule) == reduced->label.complemented
        && rexdd_is_terminal(reduced->target) == 1) {
            out = reduced;
            out->label.complemented = l.complemented ^ reduced->label.complemented;
    }

    // compatible merge with two long edges when considering terminal node (b)
    if (rexdd_is_EH(l.rule)
        && rexdd_is_one(reduced->label.rule) == !reduced->label.complemented
        && (rexdd_is_EL(reduced->label.rule) && (n==2))
        && rexdd_is_terminal(reduced->target) == 1) {
            out = reduced;
            out->label.rule = l.rule;
            out->label.complemented = !(l.complemented ^ reduced->label.complemented);
    }

    // Reduceable and compatible merge with two long edges when considering nonterminal node
        // mergeable (1)
    if ((l.rule == rexdd_rule_X && (incoming_skip==1))) {
        
        if (rexdd_is_AL(reduced->label.rule) || rexdd_is_AH(reduced->label.rule)) {
            rexdd_sanity1(reduced_skip>2, "Bad skip levels of reduced edge");
        }
        out = reduced;
        
        // mergeable (2)
    } else if ((l.rule == rexdd_rule_X && (incoming_skip>1))
                && reduced->label.rule == rexdd_rule_X){
                    out = reduced;

        // mergeable (3)
    } else if (rexdd_is_EL(l.rule)
                && rexdd_is_EL(reduced->label.rule)
                && rexdd_is_one(l.rule) == rexdd_is_one(reduced->label.rule)) {
                    out = reduced;
        
        // mergeable (4)
    } else if (rexdd_is_EH(l.rule)
                && rexdd_is_EH(reduced->label.rule)
                && rexdd_is_one(l.rule) == rexdd_is_one(reduced->label.rule)) {
                    out = reduced;

        // Unmergeable (push up one) (1)
    } else if ((l.rule == rexdd_rule_X && (incoming_skip>1))
                && reduced->label.rule != rexdd_rule_X) {

                    if (rexdd_is_AL(reduced->label.rule) || rexdd_is_AH(reduced->label.rule)) {
                        rexdd_sanity1(reduced_skip>2, "Bad skip levels of reduced edge");
                    }

                    printf("\nPush up one making handle\n");    // clean later
                    rexdd_unpacked_node_t *new_p = malloc(sizeof(rexdd_unpacked_node_t));
                    new_p->level = n+1;
                    new_p->edge[0] = *reduced;
                    new_p->edge[1] = *reduced;
                    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);

                    out->label.rule = rexdd_rule_X;
                    out->label.complemented = 0;
                    out->label.swapped = 0;
                    out->target = handle;
                    free(new_p);

        // Unmergeable (push up one) (2)
    } else if (rexdd_is_EL(l.rule)
                && !rexdd_is_EL(reduced->label.rule)
                && ((reduced->label.rule == rexdd_rule_X)
                    || 
                    ((reduced->label.rule != rexdd_rule_X)
                        && 
                        (rexdd_is_one(l.rule) == rexdd_is_one(reduced->label.rule))))
            ) {
                    if (rexdd_is_AL(reduced->label.rule) || rexdd_is_AH(reduced->label.rule)) {
                        rexdd_sanity1(reduced_skip>2, "Bad skip levels of reduced edge");
                    }

                    printf("\nPush up one making handle\n");    // clean later
                    rexdd_unpacked_node_t *new_p = malloc(sizeof(rexdd_unpacked_node_t));
                    new_p->level = n+1;
                    new_p->edge[0] = *reduced;
                    new_p->edge[0].label.rule = rexdd_rule_X;
                    new_p->edge[0].label.swapped = 0;
                    new_p->edge[0].label.complemented = rexdd_is_one(l.rule);
                    new_p->edge[0].target = rexdd_make_terminal(new_p->edge[0].target);
                    new_p->edge[1] = *reduced;
                    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);

                    if (incoming_skip == 2) {
                        out->label.rule = rexdd_rule_X;
                    } else {
                        out->label.rule = l.rule;
                    }
                    out->label.complemented = 0;
                    out->label.swapped = 0;
                    out->target = handle;
                    free(new_p);
        // Unmergeable (push up one) (3)
    } else if (rexdd_is_EH(l.rule)
                && !rexdd_is_EH(reduced->label.rule)
                && ((reduced->label.rule == rexdd_rule_X)
                    || 
                    ((reduced->label.rule != rexdd_rule_X)
                        && 
                        (rexdd_is_one(l.rule) == rexdd_is_one(reduced->label.rule))))
            ) {
                    if (rexdd_is_AL(reduced->label.rule) || rexdd_is_AH(reduced->label.rule)) {
                        rexdd_sanity1(reduced_skip>2, "Bad skip levels of reduced edge");
                    }

                    printf("\nPush up one making handle\n");    // clean later
                    rexdd_unpacked_node_t *new_p = malloc(sizeof(rexdd_unpacked_node_t));
                    new_p->level = n+1;
                    new_p->edge[1] = *reduced;
                    new_p->edge[1].label.rule = rexdd_rule_X;
                    new_p->edge[1].label.swapped = 0;
                    new_p->edge[1].label.complemented = rexdd_is_one(l.rule);
                    new_p->edge[1].target = rexdd_make_terminal(new_p->edge[1].target);
                    new_p->edge[0] = *reduced;
                    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);

                    if (incoming_skip == 2) {
                        out->label.rule = rexdd_rule_X;
                    } else {
                        out->label.rule = l.rule;
                    }
                    out->label.complemented = 0;
                    out->label.swapped = 0;
                    out->target = handle;
                    free(new_p);
    } else {
        // push up all TBD
        out = reduced;
    }
}

/* ================================================================= */

void rexdd_reduce_edge(
        rexdd_forest_t          *F,
        uint_fast32_t           m,
        uint_fast32_t           n,
        rexdd_edge_label_t      l,
        rexdd_unpacked_node_t   p,
        rexdd_edge_t            *out)
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
        printf("\nDoing check patterns\n");     // clean later
        rexdd_check_pattern(F, handle, new_p, reduced);
    }

    // normalize for the reduced edge if it has no forbidden patterns
    rexdd_normalize_edge(new_p, reduced);

    // ==============================*Fix following things*====================================================

    // Normalize the four equivalent forms
    if (reduced->target == handle) {

        /*
        * Check if it is in this forest unique table. if so, return it
        */
        printf("\tDoing insert node into UT\n");    // clean later
        uint_fast64_t H = rexdd_insert_UT(F->UT, handle & ((0x01ul << 49)-1));
        printf("\tNode handle in the UT is: %llu\n", H);    // clean later
        out = reduced;
        out->label.rule = l.rule;
        if (rexdd_is_terminal(reduced->target)) {
            H = rexdd_make_terminal(H);
        }
        out->target = H;

    } else {
        printf("\nDoing merge edge...\n");      // clean later
        rexdd_merge_edge(F, m, n, l, reduced, out);
        // if ((rexdd_get_packed_for_handle(F->M, out->target)->fourth32 & ((0x01ul << 29) - 1)) > n) {
        //     rexdd_unpacked_node_t *temp = malloc(sizeof(rexdd_unique_table_t));
        //     rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, out->target) ,temp);
        //     rexdd_reduce_edge (F, n+1, l, *temp, out);
        //     free(temp);
        // }

        printf("\tDoing insert node into UT\n");    // clean later
        uint_fast64_t H = rexdd_insert_UT(F->UT, out->target & ((0x01ul << 49)-1));
        printf("\tNode handle in the UT is: %llu\n", H);    // clean later
        if (rexdd_is_terminal(reduced->target)) {
            H = rexdd_make_terminal(H);
        }
        out->target = H;
    }

    // =====================================================================================================

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

