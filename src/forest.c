
#include <stdlib.h> // malloc/free calloc realloc
#include <stdio.h>

#include "forest.h"
#include "error.h"

/* ================================================================================================ */

void rexdd_default_forest_settings(uint_fast32_t L, rexdd_forest_settings_t *s)
{
    if (s) {
        s->num_levels = L;
        // TBD
    }
}

/****************************************************************************************************
 *
 *  BDD Forest.
 *
 ***************************************************************************************************/

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

/* ================================================================================================ */

void rexdd_free_forest(rexdd_forest_t *F)
{

    rexdd_default_forest_settings(0, &(F->S));
    rexdd_free_UT(F->UT);
    rexdd_free_nodeman(F->M);

    // remove rexdd_function_s TBD
    rexdd_done_function(F->roots);
}

/* ================================================================================================ */

void rexdd_normalize_edge(
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *out)
{
    if (rexdd_is_terminal(P->edge[0].target))  P->edge[0].label.swapped = 0;
    if (rexdd_is_terminal(P->edge[1].target))  P->edge[1].label.swapped = 0;

    /* ---------------------------------------------------------------------------------------------
     * Firstly, consider of the addresses and swapped bits order for target unpacked node's children;
     * 
     * Then take the different cases of child edges' complement bits;
     * 
     * Finally, compare the edge rules order.
     * --------------------------------------------------------------------------------------------*/

    // Low child node address and swapped bit < High child node address and swapped bit
    if ((P->edge[0].target*2+P->edge[0].target) < (P->edge[1].target*2+P->edge[1].target)) {
        if (P->edge[0].label.complemented){
            out->label.complemented = !out->label.complemented;
            rexdd_edge_com (&(P->edge[0]));
            rexdd_edge_com (&(P->edge[1]));
            
            // TBD insert normalized node into unique table
        }
    // Low child node address and swapped bit > High child node address and swapped bit
    } else if ((P->edge[0].target*2+P->edge[0].target) > (P->edge[1].target*2+P->edge[1].target)) {
        if (P->edge[1].label.complemented){
            out->label.complemented = !out->label.complemented;
            out->label.swapped = !out->label.swapped;
            rexdd_edge_com (&(P->edge[0]));
            rexdd_edge_com (&(P->edge[1]));
            rexdd_node_sw (P);
        } else {
            out->label.swapped = !out->label.swapped;
            rexdd_node_sw (P);
        }
        // TBD insert normalized node into unique table

    // Low child node address and swapped bit = High child node address and swapped bit
    } else {
        if (!P->edge[0].label.complemented && !P->edge[1].label.complemented) {
            if (P->edge[0].label.rule > P->edge[1].label.rule) {
                out->label.swapped = !out->label.swapped;
                rexdd_node_sw (P);

                // TBD insert normalized node into unique table
            }
        } else if (P->edge[0].label.complemented && P->edge[1].label.complemented) {
            if (rexdd_rule_com_t(P->edge[0].label.rule) <= rexdd_rule_com_t(P->edge[1].label.rule)) {
                out->label.complemented = !out->label.complemented;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
            } else {
                out->label.complemented = !out->label.complemented;
                out->label.swapped = !out->label.swapped;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
                rexdd_node_sw (P);
            }
            // TBD insert normalized node into unique table

        } else if (!P->edge[0].label.complemented && P->edge[1].label.complemented) {
            if (P->edge[0].label.rule > rexdd_rule_com_t(P->edge[1].label.rule)) {
                out->label.complemented = !out->label.complemented;
                out->label.swapped = !out->label.swapped;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
                rexdd_node_sw (P);

                // TBD insert normalized node into unique table
            }
        } else {
            if (rexdd_rule_com_t(P->edge[0].label.rule) <= P->edge[1].label.rule) {
                out->label.complemented = !out->label.complemented;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
            } else {
                out->label.swapped = !out->label.swapped;
                rexdd_node_sw (P);
            }
            // TBD insert normalized node into unique table

        }
    }
    // TBD make sure out->target is pointed to normalized P
}

/* ================================================================================================ */

void rexdd_check_pattern(
        rexdd_forest_t          *F,
        rexdd_node_handle_t     handle,
        rexdd_unpacked_node_t   *new_p,
        rexdd_edge_t            *reduced)
{
    // The level difference between unpacked node and both child nodes
    uint_fast32_t ln, hn;
    
    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with both edges to terminal 0 in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    if (rexdd_is_terminal(new_p->edge[0].target) && rexdd_is_terminal(new_p->edge[1].target)) {
        ln = new_p->level;
        hn = new_p->level;
        rexdd_sanity1(ln == hn, "low edge and high edge skip level error");

        /* Both edges with rexdd_rule_X (skip 0 or more nodes) and same complement bits */
        if (new_p->edge[0].label.rule == rexdd_rule_X 
            && ln >= 1
            && new_p->edge[0].label.rule == new_p->edge[1].label.rule
            && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {
            
            rexdd_set_edge(reduced,
                    rexdd_rule_X,
                    new_p->edge[0].label.complemented,
                    0,
                    new_p->edge[0].target);

        /* Both edges with rexdd_rule_X (skip 0 node) and different complement bits */
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln == 1
                    && new_p->edge[0].label.rule == new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {

                    if (!new_p->edge[0].label.complemented) {
                        rexdd_set_edge(reduced,
                                rexdd_rule_EL0,
                                new_p->edge[1].label.complemented,
                                0,
                                new_p->edge[0].target);
                    } else {
                        rexdd_set_edge(reduced,
                                rexdd_rule_EL1,
                                new_p->edge[1].label.complemented,
                                0,
                                new_p->edge[0].target);
            }

        /* 
            Low edge with rexdd_rule_X (skip more than 1 node) and different complement bits
                High edge with rexdd_rule_EL, rexdd_rule_EH or rexdd_rule_AH
                and rexdd_is_one(High edge rule) = Low edge complement bit;
            => reduced to rexdd_rule_EL or rexdd_rule_AH;

            Low edge with rexdd_rule_X (skip more than 1 node) and different complement bits
                High edge rule is not rexdd_rule_X
                and rexdd_is_one(high edge rule) = High edge complement bit;
            => reduced to rexdd_rule_X to unpacked node, and change High edge rule to rexdd_rule_X;
         */
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln>1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {
                    
                    if ((rexdd_is_EL(new_p->edge[1].label.rule)
                         || (rexdd_is_AH(new_p->edge[1].label.rule)))
                        && rexdd_is_one(new_p->edge[1].label.rule) == new_p->edge[0].label.complemented) {

                            rexdd_set_edge(reduced,
                                    new_p->edge[1].label.rule,
                                    new_p->edge[1].label.complemented,
                                    0,
                                    new_p->edge[0].target);

                    } else if (rexdd_is_EH(new_p->edge[1].label.rule)
                        && rexdd_is_one(new_p->edge[1].label.rule) == new_p->edge[0].label.complemented) {
                            if (new_p->edge[0].label.complemented) {

                                rexdd_set_edge(reduced,
                                        rexdd_rule_AH1,
                                        new_p->edge[1].label.complemented,
                                        0,
                                        new_p->edge[0].target);
                            } else {

                                rexdd_set_edge(reduced,
                                        rexdd_rule_AH0,
                                        new_p->edge[1].label.complemented,
                                        0,
                                        new_p->edge[0].target);
                            }
                    } else if (rexdd_is_AL(new_p->edge[1].label.rule)
                        && rexdd_is_one(new_p->edge[1].label.rule) == new_p->edge[0].label.complemented) {

                            rexdd_set_edge(reduced,
                                    rexdd_rule_X,
                                    0,
                                    0,
                                    handle);
                            rexdd_normalize_edge(new_p, reduced);
                    } else {
                        // ------------------------ simple for an output------------------
                        rexdd_set_edge(reduced,
                                rexdd_rule_X,
                                0,
                                0,
                                handle);
                            rexdd_normalize_edge(new_p, reduced);
                        // ---------------------------------------------------------------

                        // TBD set the unpacked node's high edge rule rexdd_rule_X, and insert it into unique table

                    }

        /* 
            High edge with rexdd_rule_X (skip more than 1 node) and different complement bits
                Low edge with rexdd_rule_EL, rexdd_rule_EH or rexdd_rule_AL
                and rexdd_is_one(Low edge rule) = High edge complement bit;
            => reduced to rexdd_rule_AL or rexdd_rule_EH;

            High edge with rexdd_rule_X (skip more than 1 node) and different complement bits
                Low edge is not rexdd_rule_X
                and rexdd_is_one(Low edge rule) = Low edge complement bit;
            => reduced to rexdd_rule_X to unpacked node, and change High edge rule to rexdd_rule_X;
         */
        } else if (new_p->edge[1].label.rule == rexdd_rule_X
                    && hn>1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented != new_p->edge[1].label.complemented) {

                    if ((rexdd_is_EH(new_p->edge[0].label.rule)
                         || (rexdd_is_AL(new_p->edge[0].label.rule)))
                        && rexdd_is_one(new_p->edge[0].label.rule) == new_p->edge[1].label.complemented) {

                            rexdd_set_edge(reduced,
                                    new_p->edge[0].label.rule,
                                    new_p->edge[0].label.complemented,
                                    0,
                                    new_p->edge[0].target);

                    } else if (rexdd_is_EL(new_p->edge[0].label.rule)
                        && rexdd_is_one(new_p->edge[0].label.rule) == new_p->edge[1].label.complemented) {
                            if (new_p->edge[1].label.complemented) {

                                rexdd_set_edge(reduced,
                                        rexdd_rule_AL1,
                                        new_p->edge[1].label.complemented,
                                        0,
                                        new_p->edge[0].target);
                            } else {

                                rexdd_set_edge(reduced,
                                        rexdd_rule_AL0,
                                        new_p->edge[1].label.complemented,
                                        0,
                                        new_p->edge[0].target);
                            }
                    } else if (rexdd_is_AL(new_p->edge[0].label.rule)
                        && rexdd_is_one(new_p->edge[0].label.rule) == new_p->edge[0].label.complemented) {

                            rexdd_set_edge(reduced,
                                    rexdd_rule_X,
                                    0,
                                    0,
                                    handle);
                            rexdd_normalize_edge(new_p, reduced);
                    } else {
                        // ------------------------ simple for an output------------------
                        rexdd_set_edge(reduced,
                                rexdd_rule_X,
                                0,
                                0,
                                handle);
                            rexdd_normalize_edge(new_p, reduced);
                        // ---------------------------------------------------------------

                        // TBD set the unpacked node's high edge rule rexdd_rule_X, and insert it into unique table

                    }


        /*
            Low edge with rexdd_rule_X (skip more than 1 node) and same complement bits
                High edge with rexdd_rule_EL, rexdd_rule_EH, rexdd_rule_AL or rexdd_rule_AH 
                and rexdd_is_one(High edge rule) = High edge complement bit;
            => reduced to rexdd_rule_X with same complement bit
        */
        } else if (new_p->edge[0].label.rule == rexdd_rule_X
                    && ln > 1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {

                        if (rexdd_is_one(new_p->edge[1].label.rule) == new_p->edge[0].label.complemented) {

                            rexdd_set_edge(reduced,
                                    rexdd_rule_X,
                                    new_p->edge[0].label.complemented,
                                    0,
                                    new_p->edge[0].target);
                    
                        } else {
                            rexdd_set_edge(reduced,
                                    rexdd_rule_X,
                                    0,
                                    0,
                                    handle);
                            rexdd_normalize_edge(new_p, reduced);
                        }
        /*
            High edge with rexdd_rule_X (skip more than 1 node) and same complement bits
                Low edge with rexdd_rule_EL, rexdd_rule_EH, rexdd_rule_AL or rexdd_rule_AH 
                and rexdd_is_one(Low edge rule) = Low edge complement bit;
            => reduced to rexdd_rule_X with same complement bit
        */
        } else if (new_p->edge[1].label.rule == rexdd_rule_X
                    && hn > 1
                    && new_p->edge[0].label.rule != new_p->edge[1].label.rule
                    && new_p->edge[0].label.complemented == new_p->edge[1].label.complemented) {

                        if (rexdd_is_one(new_p->edge[0].label.rule) == new_p->edge[1].label.complemented) {

                            rexdd_set_edge(reduced,
                                    rexdd_rule_X,
                                    new_p->edge[0].label.complemented,
                                    0,
                                    new_p->edge[0].target);
                            
                        } else {
                            rexdd_set_edge(reduced,
                                    rexdd_rule_X,
                                    0,
                                    0,
                                    handle);
                            rexdd_normalize_edge(new_p, reduced);
                        }
        }else {
            reduced->target = handle;
        }
    }

     /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with Low edge to terminal 0 and High edge to nonterminal in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    else if (rexdd_is_terminal(new_p->edge[0].target) && !rexdd_is_terminal(new_p->edge[1].target)) {
        ln = new_p->level;
        hn = new_p->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, new_p->edge[1].target));

        rexdd_set_edge(reduced,
                rexdd_rule_X,
                new_p->edge[1].label.complemented,
                new_p->edge[1].label.swapped,
                new_p->edge[1].target);
        
        rexdd_sanity1(ln != hn, "low edge and high edge skip level error");

        // pattern (b) & (c)
        if (new_p->edge[0].label.rule == rexdd_rule_X
            && ln > 1
            && ln > hn) {

            if (!new_p->edge[0].label.complemented) {
                if ((new_p->edge[1].label.rule == rexdd_rule_X && (hn == 1))
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
    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with High edge to terminal 0 and Low edge to nonterminal in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    else if (!rexdd_is_terminal(new_p->edge[0].target) && rexdd_is_terminal(new_p->edge[1].target)) {
        ln = new_p->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, new_p->edge[0].target));
        hn = new_p->level;

        reduced->target = new_p->edge[0].target;
        reduced->label.swapped = new_p->edge[0].label.swapped;
        reduced->label.complemented = new_p->edge[0].label.complemented;

        rexdd_sanity1(ln != hn, "low edge and high edge skip level error");

        // pattern (e) & (f)
        if (new_p->edge[1].label.rule == rexdd_rule_X
            && hn > 1
            && hn > ln
            ) {
            if (!new_p->edge[1].label.complemented) {
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
    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with both edges to nonterminal in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    else if (!rexdd_is_terminal(new_p->edge[0].target) && !rexdd_is_terminal(new_p->edge[1].target)
                && new_p->edge[1].target == new_p->edge[0].target) {
        ln = new_p->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, new_p->edge[0].target));
        hn = new_p->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, new_p->edge[1].target));
    
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
            // normalize for the reduced edge
            rexdd_normalize_edge(new_p, reduced);
        }
    }
}

/* ================================================================================================ */

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

    if (l.complemented) rexdd_edge_com (reduced);

    // Unreduceable and compatible merge
    if (reduced->label.rule == rexdd_rule_X && (reduced_skip==0)) {
            reduced->label.rule = l.rule;
            out = reduced;
    }

    // compatible merge with two long edges when considering terminal node (a)
    if (reduced->label.rule == rexdd_rule_X
        && l.rule != rexdd_rule_X
        && rexdd_is_one(l.rule) == reduced->label.complemented
        && rexdd_is_terminal(reduced->target)) {
            out = reduced;
            out->label.complemented = l.complemented ^ reduced->label.complemented;
    }

    // compatible merge with two long edges when considering terminal node (b)
    if (rexdd_is_EH(l.rule)
        && rexdd_is_one(reduced->label.rule) == !reduced->label.complemented
        && (rexdd_is_EL(reduced->label.rule) && (n==2))
        && rexdd_is_terminal(reduced->target)) {
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

/* ================================================================================================ */

void rexdd_reduce_edge(
        rexdd_forest_t          *F,
        uint_fast32_t           m,
        uint_fast32_t           n,
        rexdd_edge_label_t      l,
        rexdd_unpacked_node_t   p,
        rexdd_edge_t            *out)
{
    rexdd_sanity1(n == p.level, "Bad target node level");
    rexdd_sanity1(m > n, "Bad incoming edge root level");

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

    if (l.swapped) {
        rexdd_node_sw (new_p);
    }

    // check if it has a terminal or nonterminal pattern
    rexdd_edge_t *reduced = malloc(sizeof(rexdd_edge_t));
    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, new_p);
    reduced->target = handle;
    reduced->label.rule = rexdd_rule_X;
    reduced->label.swapped = l.swapped;
    reduced->label.complemented = l.complemented;

    // avoid push up all for the AL or AH cases in merge part
    if (!rexdd_is_AL(l.rule) && !rexdd_is_AH(l.rule)) {
        printf("\nDoing check patterns\n");     // clean later
        rexdd_check_pattern(F, handle, new_p, reduced);
    }

    
    if (reduced->target == handle) {
        // insert (into normalize)
        printf("\tDoing insert node into UT\n");    // clean later
        uint_fast64_t H = rexdd_insert_UT(F->UT, handle & ((0x01ul << 49)-1));
        printf("\tNode handle in the UT is: %llu\n", H);    // clean later
        out = reduced;
        out->label.rule = l.rule;
        if (rexdd_is_terminal(handle)) {
            H = rexdd_make_terminal(H);
            out->target = H;
        }
    } else {
        printf("\nDoing merge edge...\n");      // clean later
        rexdd_merge_edge(F, m, n, l, reduced, out);
        // if ((rexdd_get_packed_for_handle(F->M, out->target)->fourth32 & ((0x01ul << 29) - 1)) > n) {
        //     rexdd_unpacked_node_t *temp = malloc(sizeof(rexdd_unique_table_t));
        //     rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, out->target) ,temp);
        //     rexdd_reduce_edge (F, n+1, l, *temp, out);
        //     free(temp);
        // }

        // insert (into normalize)
        printf("\tDoing insert node into UT\n");    // clean later
        uint_fast64_t H = rexdd_insert_UT(F->UT, out->target & ((0x01ul << 49)-1));
        printf("\tNode handle in the UT is: %llu\n", H);    // clean later
        if (rexdd_is_terminal(out->target)) {
            H = rexdd_make_terminal(H);
            out->target = H;
        }
        
    }

    free(new_p);
    free(reduced);

}


bool rexdd_eval(
        rexdd_forest_t          *F,
        rexdd_edge_t            *e,
        uint32_t                m,
        bool                    vars[])
{
    rexdd_sanity1(m>=0, "Root level of edge can not be less than 0");

    uint32_t k;
    if (rexdd_is_terminal(e->target)) {
        k = 0;
    } else {
        k = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e->target));
    }
    rexdd_sanity1(k<=m, "Target node level of edge must be less than root level");

    bool result;
    if (k == m) {
        if (m == 0){
            result = e->label.complemented ^ 0;
        } else {
            if (!rexdd_is_terminal(e->target)) {
                bool next = e->label.swapped ^ vars[m];
                rexdd_edge_t *e_next = malloc(sizeof(rexdd_edge_t));
                if (next) {
                    rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->UT->M, e->target), &(e_next->label));
                    e_next->target = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, e->target));
                } else {
                    rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->UT->M, e->target), &(e_next->label));
                    e_next->target = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, e->target));
                }
                result = e->label.complemented ^ rexdd_eval(F, e_next, m-1, vars);
                free(e_next);
            } else {
                result = e->label.complemented ^ 0;
            }
        }
    } else {
        if (e->label.rule == rexdd_rule_X) {
            result = rexdd_eval(F, e, k, vars);
        } else {

            bool vars_and = 1, vars_or = 0;
            for (uint32_t i=k+1; i<=m; i++) {
                vars_and = vars_and & vars[i];
                vars_or = vars_or | vars[i];
            }

            if (rexdd_is_EL(e->label.rule)) {
                if (vars_and) {
                    result = rexdd_eval(F, e, k, vars);
                } else {
                    result = rexdd_is_one(e->label.rule);
                }
            }
            if (rexdd_is_AH(e->label.rule)) {
                if (vars_and) {
                    result = rexdd_is_one(e->label.rule);
                } else {
                    result = rexdd_eval(F, e, k, vars);
                }
            }

            if (rexdd_is_AL(e->label.rule)) {
                if (vars_or) {
                    result = rexdd_eval(F, e, k, vars);
                } else {
                    result = rexdd_is_one(e->label.rule);
                }
            }
            if (rexdd_is_EH(e->label.rule)) {
                if (vars_or) {
                    result = rexdd_is_one(e->label.rule);
                } else {
                    result = rexdd_eval(F, e, k, vars);
                }
            }
            
        }
        
    }

    return result;
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

