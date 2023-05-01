
#include <stdlib.h> // malloc/free calloc realloc
#include <stdio.h>
#include <time.h>

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
    F->CT = malloc(sizeof(rexdd_comp_table_t));
    rexdd_init_CT(F->CT);
    // Initialize list of root edges (empty)
    F->roots = NULL;

}

/* ================================================================================================ */

void rexdd_free_forest(rexdd_forest_t *F)
{

    rexdd_default_forest_settings(0, &(F->S));
    rexdd_free_UT(F->UT);
    free(F->UT);
    rexdd_free_nodeman(F->M);
    free(F->M);
    rexdd_free_CT(F->CT);
    free(F->CT);
}

/* ================================================================================================ */

void rexdd_normalize_node(
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *out)
{
    rexdd_sanity1(out, "null edge to the normalized node");

    if (rexdd_is_terminal(P->edge[0].target))  P->edge[0].label.swapped = 0;
    if (rexdd_is_terminal(P->edge[1].target))  P->edge[1].label.swapped = 0;
    out->label.complemented = 0;
    out->label.swapped = 0;
    /* ---------------------------------------------------------------------------------------------
     * Firstly, consider of the addresses and swapped bits order for target unpacked node's children;
     *
     * Then take the different cases of child edges' complement bits;
     *
     * Finally, compare the edge rules order.
     * --------------------------------------------------------------------------------------------*/
#if defined REXBDD || defined CS_QBDD || defined CS_FBDD
    if (P->edge[0].target != P->edge[1].target) {
        out->label.swapped = P->edge[0].target > P->edge[1].target;
    } else if (P->edge[0].label.swapped != P->edge[1].label.swapped) {
        out->label.swapped = P->edge[0].label.swapped > P->edge[1].label.swapped;
    } else {
        rexdd_rule_t ruleL, ruleH;
        if (P->edge[0].label.complemented) {
            ruleL = rexdd_rule_com_t(P->edge[0].label.rule);
        } else {
            ruleL = P->edge[0].label.rule;
        }
        if (P->edge[1].label.complemented) {
            ruleH = rexdd_rule_com_t(P->edge[1].label.rule);
        } else {
            ruleH = P->edge[1].label.rule;
        }
        out->label.swapped = ruleL > ruleH;
    }
    if (out->label.swapped) {
        rexdd_node_sw (P);
    }
    out->label.complemented = P->edge[0].label.complemented;
    if (out->label.complemented) {
        rexdd_edge_com(&(P->edge[0]));
        rexdd_edge_com(&(P->edge[1]));
    }
#elif defined C_QBDD || defined C_FBDD || defined CESRBDD
    out->label.complemented = P->edge[0].label.complemented;
    if (out->label.complemented) {
        rexdd_edge_com(&(P->edge[0]));
        rexdd_edge_com(&(P->edge[1]));
    }
#elif defined S_QBDD || defined S_FBDD
    if (P->edge[0].target != P->edge[1].target) {
        out->label.swapped = P->edge[0].target > P->edge[1].target;
    } else if (P->edge[0].label.swapped != P->edge[1].label.swapped) {
        out->label.swapped = P->edge[0].label.swapped > P->edge[1].label.swapped;
    } else {
        out->label.swapped = P->edge[0].label.rule > P->edge[1].label.rule;
    }
    if (out->label.swapped) {
        rexdd_node_sw (P);
    }
#else // QBDD || FBDD || ZBDD || ESRBDD

#endif
}

/* ================================================================================================ */

void rexdd_reduce_node(
        rexdd_forest_t          *F,
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *reduced)
{
    rexdd_sanity1(reduced, "null reduced esge");

#if defined C_QBDD || defined CS_QBDD || defined C_FBDD || defined CS_FBDD || defined CESRBDD || defined REXBDD
    // If the target node of unpacked node P's child edge is terminal 1, inverse the complement bit of this child edge
    if (rexdd_is_terminal(P->edge[0].target)
        && rexdd_terminal_value(P->edge[0].target)) {
            P->edge[0].label.complemented = !P->edge[0].label.complemented;
            P->edge[0].target = rexdd_make_terminal(0);
        }
    if (rexdd_is_terminal(P->edge[1].target)
        && rexdd_terminal_value(P->edge[1].target)) {
            P->edge[1].label.complemented = !P->edge[1].label.complemented;
            P->edge[1].target = rexdd_make_terminal(0);
        }
#endif

#if defined REXBDD || defined CESRBDD
    /* -----------------------------------------------------------------------------------------
        Constant edge: the edges <ELc, 0, c, 0>, <EHc, 0, c, 0>, <ALc, 0, c, 0>, <AHc, 0, c, 0>
        with any complement bit c can be uniformly represented by <X, 0, c, 0>
            After this, there are only two kinds of edges to terminal 0:

                1. edge rule rexdd_rule_X with any complement bit
                2. edge rule not rexdd_rule_X with different complement bit,
                    for example <EL0, 0, 1, 0>
    */
    if (rexdd_is_terminal(P->edge[0].target)
        && P->edge[0].label.rule != rexdd_rule_X
        && (rexdd_is_one(P->edge[0].label.rule) == P->edge[0].label.complemented)) {
            P->edge[0].label.rule = rexdd_rule_X;
        }
    if (rexdd_is_terminal(P->edge[1].target)
        && P->edge[1].label.rule != rexdd_rule_X
        && (rexdd_is_one(P->edge[1].label.rule) == P->edge[1].label.complemented)) {
            P->edge[1].label.rule = rexdd_rule_X;
        }
#endif

#if defined ZBDD || defined ESRBDD
    if (rexdd_is_terminal(P->edge[0].target) && !rexdd_terminal_value(P->edge[0].target)
        && P->edge[0].label.rule != rexdd_rule_X) {
            P->edge[0].label.rule = rexdd_rule_X;
        }
    if (rexdd_is_terminal(P->edge[1].target) && !rexdd_terminal_value(P->edge[1].target)
        && P->edge[1].label.rule != rexdd_rule_X) {
            P->edge[1].label.rule = rexdd_rule_X;
        }
#endif

#ifdef REXBDD
    // The level difference between unpacked node and both child nodes
    uint_fast32_t ln, hn;

    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with both edges to terminal 0 in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    if (rexdd_is_terminal(P->edge[0].target) && rexdd_is_terminal(P->edge[1].target)) {
        ln = P->level;
        hn = P->level;
        rexdd_sanity1(ln == hn, "low edge and high edge skip level error");

        /*
            Meta-edge: Constant <C, 0, c, 0> = <X, 0, c, 0>
                Both edges with rexdd_rule_X (skip 0 or more nodes) and same complement bits
        */
        if (P->edge[0].label.rule == rexdd_rule_X
            && P->edge[1].label.rule == rexdd_rule_X
            && ln >= 1
            && P->edge[0].label.complemented == P->edge[1].label.complemented) {

            rexdd_set_edge(reduced,
                    rexdd_rule_X,
                    P->edge[0].label.complemented,
                    0,
                    P->edge[0].target);

        /*
            Meta-edge: Bottom variable <B, 0, c, 0> = <ELc, 0, not_c, 0>
                Both edges with rexdd_rule_X (skip 0 node) and different complement bits
        */
        } else if (P->edge[0].label.rule == rexdd_rule_X
                    && P->edge[1].label.rule == rexdd_rule_X
                    && ln == 1
                    && P->edge[0].label.complemented != P->edge[1].label.complemented) {

                if (!P->edge[0].label.complemented) {
                    rexdd_set_edge(reduced,
                            rexdd_rule_EL0,
                            P->edge[1].label.complemented,
                            0,
                            P->edge[0].target);
                } else {
                    rexdd_set_edge(reduced,
                            rexdd_rule_EL1,
                            P->edge[1].label.complemented,
                            0,
                            P->edge[0].target);
                }

        /*
            Meta-edge: anD (conjunction) <D, 0, c, 0> = <ELc, 0, not_c, 0>
                Low edge with rexdd_rule_X (skip 1 or more nodes) and High edge with not rexdd_rule_X

                    1. High edge with rule EL, different complement bit to Low edge;
                    2. High edge with rule EH or AH (skip 2 or more nodes), the same
                       complement bit with Low edge;
         */
        } else if (P->edge[0].label.rule == rexdd_rule_X
                    && P->edge[1].label.rule != rexdd_rule_X
                    && ln>1
                    && ((P->edge[0].label.complemented != P->edge[1].label.complemented
                            && rexdd_is_EL(P->edge[1].label.rule))
                        ||
                        (P->edge[0].label.complemented == P->edge[1].label.complemented
                            && ((rexdd_is_EH(P->edge[1].label.rule) && hn==2)
                                ||
                                (rexdd_is_AH(P->edge[1].label.rule) && hn>2))))) {

                if (!P->edge[0].label.complemented) {
                    rexdd_set_edge(reduced,
                            rexdd_rule_EL0,
                            1,
                            0,
                            P->edge[0].target);
                } else {
                    rexdd_set_edge(reduced,
                            rexdd_rule_EL1,
                            0,
                            0,
                            P->edge[0].target);
                }

        /*
            Meta-edge: oR (disjunction) <R, 0, c, 0> = <EHnot_c, 0, c, 0>
                High edge with rexdd_rule_X (skip 1 or more nodes) and Low edge with not rexdd_rule_X

                    1. Low edge with rule EH, different complement bit to High edge;
                    2. Low edge with rule EL or Al (skip 2 or more nodes), the same
                       complement bit with High edge;
         */
        } else if (P->edge[1].label.rule == rexdd_rule_X
                    && P->edge[0].label.rule != rexdd_rule_X
                    && hn>1
                    && ((P->edge[0].label.complemented != P->edge[1].label.complemented
                            && rexdd_is_EH(P->edge[0].label.rule))
                        ||
                        (P->edge[0].label.complemented == P->edge[1].label.complemented
                            && ((rexdd_is_EL(P->edge[0].label.rule) && ln==2)
                                ||
                                (rexdd_is_AL(P->edge[0].label.rule) && ln>2))))) {

            if (!P->edge[1].label.complemented) {
                rexdd_set_edge(reduced,
                        rexdd_rule_EH0,
                        1,
                        0,
                        P->edge[0].target);
            } else {
                rexdd_set_edge(reduced,
                        rexdd_rule_EH1,
                        0,
                        0,
                        P->edge[0].target);
            }

        /*
            new_p can not be represented by a long edge, we need to normalize it

                1. The normalized unpacked node may be changed, the reason may be:

                    a). Low or High edge is changed to meta-edge Constant, then it already
                        met the canonicity (not need to do swap or complement);
                    b). Low or High edge is not changed to meta-edge Constant, but it needs
                        to do swap or complement for canonicity;
                    c). Low or High edge is changed to meta-edge Constant, then it also
                        needs to do swap or complement for canonicity.

                2. The normalized unpacked node is not changed, because its Low or High edge
                   is not changed to meta-edge Constant and it already met the canonicity. So
                   the reduced edge can still target to the original handle.

                No matter it changed or not, we can call rexdd_insert_UT to check if it is duplicated,
                then insert it to the unique table.

                If swapped new_p is a pattern node, then normalize new_p without swapping to ensure
                new_p[0] complement bit 0.
        */
        } else {
            if (P->edge[0].label.rule == rexdd_rule_X
                && ((P->edge[0].label.complemented != P->edge[1].label.complemented
                            && rexdd_is_EH(P->edge[1].label.rule))
                        ||
                        (P->edge[0].label.complemented == P->edge[1].label.complemented
                            && ((rexdd_is_EL(P->edge[1].label.rule) && hn==2)
                                ||
                                (rexdd_is_AL(P->edge[1].label.rule) && hn>2))))) {
                reduced->label.swapped = 0;
                if (P->edge[0].label.complemented) {
                    reduced->label.complemented = 1;
                    rexdd_edge_com(&(P->edge[0]));
                    rexdd_edge_com(&(P->edge[1]));
                } else {
                    reduced->label.complemented = 0;
                }
            } else if (P->edge[1].label.rule == rexdd_rule_X
                        && ((P->edge[0].label.complemented != P->edge[1].label.complemented
                                    && rexdd_is_EL(P->edge[0].label.rule))
                                ||
                                (P->edge[0].label.complemented == P->edge[1].label.complemented
                                    && ((rexdd_is_EH(P->edge[0].label.rule) && ln==2)
                                        ||
                                        (rexdd_is_AH(P->edge[0].label.rule) && ln>2))))) {
                reduced->label.swapped = 0;
                if (P->edge[0].label.complemented) {
                    reduced->label.complemented = 1;
                    rexdd_edge_com(&(P->edge[0]));
                    rexdd_edge_com(&(P->edge[1]));
                } else {
                    reduced->label.complemented = 0;
                }
            } else {
                // rexdd_normalize_node will set edge "*reduced" swap bit and complement bit
                rexdd_normalize_node(P, reduced);
            }

            reduced->label.rule = rexdd_rule_X;
            rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
            handle = rexdd_insert_UT(F->UT, handle);
            reduced->target = handle;
        }
    }

     /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with Low edge to terminal 0 and High edge to nonterminal in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    else if (rexdd_is_terminal(P->edge[0].target) && !rexdd_is_terminal(P->edge[1].target)) {
        ln = P->level;
        hn = P->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, P->edge[1].target));

        /*
            Low edge to terminal 0 with rule rexdd_rule_X and complement bit t

                1. High edge to nonterminal ndoe q with rule rexdd_rule_X (skip 0 node),
                   swapped bit s and complement bit c;
                2. High edge to nonterminal ndoe q with rule rexdd_rule_ELt (skip 1 or more node),
                   swapped bit s and complement bit c;

            the unpacked node can be represented by a long edge <ELt, s, c, q>
        */
       if (P->edge[0].label.rule == rexdd_rule_X
            && (((P->edge[1].label.rule == rexdd_rule_X) && hn==1)
                ||
                (rexdd_is_EL(P->edge[1].label.rule) && hn>1
                && rexdd_is_one(P->edge[1].label.rule) == P->edge[0].label.complemented))) {
            if (P->edge[0].label.complemented) {
                rexdd_set_edge(reduced,
                        rexdd_rule_EL1,
                        P->edge[1].label.complemented,
                        P->edge[1].label.swapped,
                        P->edge[1].target);
            } else {
                rexdd_set_edge(reduced,
                        rexdd_rule_EL0,
                        P->edge[1].label.complemented,
                        P->edge[1].label.swapped,
                        P->edge[1].target);
            }
        } else {
            if (P->edge[0].label.rule == rexdd_rule_X
                && rexdd_is_EH(P->edge[1].label.rule) && hn>1
                && rexdd_is_one(P->edge[1].label.rule) == P->edge[0].label.complemented) {
                reduced->label.swapped = 0;
                if (P->edge[0].label.complemented) {
                    reduced->label.complemented = 1;
                    rexdd_edge_com(&(P->edge[0]));
                    rexdd_edge_com(&(P->edge[1]));
                } else {
                    reduced->label.complemented = 0;
                }
            } else {
                rexdd_normalize_node(P, reduced);
            }

            reduced->label.rule = rexdd_rule_X;
            rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
            handle = rexdd_insert_UT(F->UT, handle);
            reduced->target = handle;
        }
    }
    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with High edge to terminal 0 and Low edge to nonterminal in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    else if (!rexdd_is_terminal(P->edge[0].target) && rexdd_is_terminal(P->edge[1].target)) {
        ln = P->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, P->edge[0].target));
        hn = P->level;

        /*
            High edge to terminal 0 with rule rexdd_rule_X and complement bit t

                1. Low edge to nonterminal ndoe q with rule rexdd_rule_X (skip 0 node),
                   swapped bit s and complement bit c;
                2. Low edge to nonterminal ndoe q with rule rexdd_rule_ELt (skip 1 or more node),
                   swapped bit s and complement bit c;

            the unpacked node can be represented by a long edge <ELt, s, c, q>
        */
        if (P->edge[1].label.rule == rexdd_rule_X
            && (((P->edge[0].label.rule == rexdd_rule_X) && ln==1)
                ||
                (rexdd_is_EH(P->edge[0].label.rule) && ln>1
                && rexdd_is_one(P->edge[0].label.rule) == P->edge[1].label.complemented))) {
            if (P->edge[1].label.complemented) {
                rexdd_set_edge(reduced,
                        rexdd_rule_EH1,
                        P->edge[0].label.complemented,
                        P->edge[0].label.swapped,
                        P->edge[0].target);
            } else {
                rexdd_set_edge(reduced,
                        rexdd_rule_EH0,
                        P->edge[0].label.complemented,
                        P->edge[0].label.swapped,
                        P->edge[0].target);
            }
        } else {
            if (P->edge[1].label.rule == rexdd_rule_X
                        && rexdd_is_EL(P->edge[0].label.rule) && ln>1
                        && rexdd_is_one(P->edge[0].label.rule) == P->edge[1].label.complemented) {
                reduced->label.swapped = 0;
                if (P->edge[0].label.complemented) {
                    reduced->label.complemented = 1;
                    rexdd_edge_com(&(P->edge[0]));
                    rexdd_edge_com(&(P->edge[1]));
                } else {
                    reduced->label.complemented = 0;
                }
            } else {
                rexdd_normalize_node(P, reduced);
            }
            reduced->label.rule = rexdd_rule_X;
            rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
            handle = rexdd_insert_UT(F->UT, handle);
            reduced->target = handle;
        }
    }

    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with both edges to the same nonterminal in canonical RexBDDs
     * --------------------------------------------------------------------------------------------*/
    else if (!rexdd_is_terminal(P->edge[0].target) && !rexdd_is_terminal(P->edge[1].target)
                && P->edge[1].target == P->edge[0].target) {
        ln = P->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, P->edge[0].target));
        hn = P->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, P->edge[1].target));

        /*
            Low and High edges to the same nonterminal node q with the same complement bit c and swapped
            bit s

                1. High and Low edges with rule rexdd_rule_X (skip 0 or more nodes)
                        The unpacked node can be represented by a long edge <X, s, c, q>;
                2. Low edges with rule rexdd_rule_X (skip 0 or more nodes), High edge
                   with rule rexdd_rule_EHt (skip 1 node) or rexdd_rule_AHt (skip more
                   than 2 nodes)
                        The unpacked node can be represented by a long edge <AHt, s, c, q>;
                3. High edges with rule rexdd_rule_X (skip 0 or more nodes), Low edge
                   with rule rexdd_rule_ELt (skip 1 node) or rexdd_rule_ALt (skip more
                   than 2 nodes)
                        The unpacked node can be represented by a long edge <ALt, s, c, q>;
        */
       if ((P->edge[0].label.complemented == P->edge[1].label.complemented)
            && (P->edge[0].label.swapped == P->edge[1].label.swapped)
            && P->edge[0].label.rule == rexdd_rule_X
            && P->edge[1].label.rule == rexdd_rule_X) {
            rexdd_set_edge(reduced,
                    rexdd_rule_X,
                    P->edge[0].label.complemented,
                    P->edge[0].label.swapped,
                    P->edge[0].target);

        } else if ((P->edge[0].label.complemented == P->edge[1].label.complemented)
                    && (P->edge[0].label.swapped == P->edge[1].label.swapped)
                    && P->edge[0].label.rule == rexdd_rule_X
                    && ((rexdd_is_EH(P->edge[1].label.rule) && hn==2)
                        ||(rexdd_is_AH(P->edge[1].label.rule) && hn>2))
            ) {
            if (rexdd_is_one(P->edge[1].label.rule)) {
                rexdd_set_edge(reduced,
                        rexdd_rule_AH1,
                        P->edge[0].label.complemented,
                        P->edge[0].label.swapped,
                        P->edge[0].target);
            } else {
                rexdd_set_edge(reduced,
                        rexdd_rule_AH0,
                        P->edge[0].label.complemented,
                        P->edge[0].label.swapped,
                        P->edge[0].target);
            }

        } else if ((P->edge[0].label.complemented == P->edge[1].label.complemented)
                    && (P->edge[0].label.swapped == P->edge[1].label.swapped)
                    && P->edge[1].label.rule == rexdd_rule_X
                    && ((rexdd_is_EL(P->edge[0].label.rule) && ln==2)
                        ||(rexdd_is_AL(P->edge[0].label.rule) && ln>2))) {
            if (rexdd_is_one(P->edge[0].label.rule)) {
                rexdd_set_edge(reduced,
                        rexdd_rule_AL1,
                        P->edge[0].label.complemented,
                        P->edge[0].label.swapped,
                        P->edge[0].target);
            } else {
                rexdd_set_edge(reduced,
                        rexdd_rule_AL0,
                        P->edge[0].label.complemented,
                        P->edge[0].label.swapped,
                        P->edge[0].target);
            }
        } else {
            if ((P->edge[0].label.complemented == P->edge[1].label.complemented)
                && (P->edge[0].label.swapped == P->edge[1].label.swapped)
                && P->edge[0].label.rule == rexdd_rule_X
                && ((rexdd_is_EL(P->edge[1].label.rule) && hn==2)
                    ||(rexdd_is_AL(P->edge[1].label.rule) && hn>2))) {
                reduced->label.swapped = 0;
                if (P->edge[0].label.complemented) {
                    reduced->label.complemented = 1;
                    rexdd_edge_com(&(P->edge[0]));
                    rexdd_edge_com(&(P->edge[1]));
                } else {
                    reduced->label.complemented = 0;
                }
            } else if ((P->edge[0].label.complemented == P->edge[1].label.complemented)
                && (P->edge[0].label.swapped == P->edge[1].label.swapped)
                && P->edge[1].label.rule == rexdd_rule_X
                && ((rexdd_is_EH(P->edge[0].label.rule) && ln==2)
                    ||(rexdd_is_AH(P->edge[0].label.rule) && ln>2))) {
                reduced->label.swapped = 0;
                if (P->edge[0].label.complemented) {
                    reduced->label.complemented = 1;
                    rexdd_edge_com(&(P->edge[0]));
                    rexdd_edge_com(&(P->edge[1]));
                } else {
                    reduced->label.complemented = 0;
                }
            } else {
                rexdd_normalize_node(P, reduced);
            }
            reduced->label.rule = rexdd_rule_X;
            rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
            handle = rexdd_insert_UT(F->UT, handle);
            reduced->target = handle;
        }
    }
    /* ---------------------------------------------------------------------------------------------
     * Normalize the node with child edges to different nonterminals
     * --------------------------------------------------------------------------------------------*/
    else {
        rexdd_normalize_node(P, reduced);
        reduced->label.rule = rexdd_rule_X;
        rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
        handle = rexdd_insert_UT(F->UT, handle);
        reduced->target = handle;
    }
#endif


#if defined C_QBDD || defined CS_QBDD || defined QBDD || defined S_QBDD
    reduced->label.rule = rexdd_rule_X;

    rexdd_normalize_node(P, reduced);
    reduced->target = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, P));
#endif

#if defined C_FBDD || defined CS_FBDD || defined FBDD || defined S_FBDD
    reduced->label.rule = rexdd_rule_X;

    if (P->edge[0].target == P->edge[1].target
        && P->edge[0].label.complemented == P->edge[1].label.complemented
        && P->edge[0].label.swapped == P->edge[1].label.swapped) {
        reduced->label.complemented = P->edge[0].label.complemented;
        reduced->label.swapped = P->edge[0].label.swapped;
        reduced->target = P->edge[0].target;
    } else {
        rexdd_normalize_node(P, reduced);
        reduced->target = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, P));
    }
#endif

#ifdef ZBDD
    reduced->label.rule = rexdd_rule_X;
    reduced->label.complemented = 0;
    reduced->label.swapped = 0;
    if (P->edge[1].target == rexdd_make_terminal(0)) {
        reduced->target = P->edge[0].target;
        if (rexdd_is_terminal(P->edge[0].target) && !rexdd_terminal_value(P->edge[0].target)) {
            reduced->label.rule = rexdd_rule_X;
        } else {
            reduced->label.rule = rexdd_rule_EH0;
        }
    } else {
        reduced->target = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, P));
    }
#endif

#if defined ESRBDD || defined CESRBDD
    uint_fast32_t ln, hn;
    if (rexdd_is_terminal(P->edge[0].target)) {
        ln = P->level;
    } else {
        ln = P->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, P->edge[0].target));
    }
    if (rexdd_is_terminal(P->edge[1].target)) {
        hn = P->level;
    } else {
        hn = P->level - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, P->edge[1].target));
    }

    reduced->label.complemented = 0;
    reduced->label.swapped = 0;

    if (P->edge[0].target == P->edge[1].target
        && P->edge[0].label.rule == rexdd_rule_X
        && P->edge[1].label.rule == rexdd_rule_X
        && P->edge[0].label.complemented == P->edge[1].label.complemented) {
        reduced->label.rule = rexdd_rule_X;
        reduced->label.complemented = P->edge[0].label.complemented;
        reduced->target = P->edge[1].target;
    } else if (P->edge[1].target == rexdd_make_terminal(0) && P->edge[1].label.rule == rexdd_rule_X
                && ((rexdd_is_EH(P->edge[0].label.rule)
                    && rexdd_is_one(P->edge[0].label.rule) == P->edge[1].label.complemented)
                    || (P->edge[0].label.rule == rexdd_rule_X && ln == 1))) {
        if (P->edge[1].label.complemented){
            reduced->label.rule = rexdd_rule_EH1;
        } else {
            reduced->label.rule = rexdd_rule_EH0;
        }
        reduced->label.complemented = P->edge[0].label.complemented;
        reduced->target = P->edge[0].target;
    } else if (P->edge[0].target == rexdd_make_terminal(0) && P->edge[0].label.rule == rexdd_rule_X
                && ((rexdd_is_EL(P->edge[1].label.rule)
                    && rexdd_is_one(P->edge[1].label.rule) == P->edge[0].label.complemented)
                    || (P->edge[1].label.rule == rexdd_rule_X && hn == 1))) {
        if (P->edge[0].label.complemented){
            reduced->label.rule = rexdd_rule_EL1;
        } else {
            reduced->label.rule = rexdd_rule_EL0;
        }
        reduced->label.complemented = P->edge[1].label.complemented;
        reduced->target = P->edge[1].target;
    } else {
        reduced->label.rule = rexdd_rule_X;
        rexdd_normalize_node(P,reduced);
        reduced->target = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, P));
    }

    if (reduced->target == rexdd_make_terminal(0)) {
        if (reduced->label.rule != rexdd_rule_X
            && reduced->label.complemented == rexdd_is_one(reduced->label.rule)) {
            reduced->label.rule = rexdd_rule_X;
            }
    }
#endif


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
    rexdd_sanity1(reduced, "null reduced edge");
    rexdd_sanity1(out, "null merged edge");

    uint32_t incoming_skip = m - n;
    uint32_t reduced_skip;
    if (rexdd_is_terminal(reduced->target)) {
        reduced_skip = n;
    } else {
        reduced_skip = n - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, reduced->target));
    }

    if (l.complemented) rexdd_edge_com (reduced);

    /* ---------------------------------------------------------------------------------------------
     * Compatible merge
     *
     *      1. Incoming edge rule is rexdd_rule_X (skip 0 node);
     *      2. Incoming edge rule is rexdd_rule_X (skip 1 or more nodes), edge "*reduced" rule is
     *         rexdd_rule_X;
     *      3. Incoming edge rule is rexdd_rule_EL, edge "*reduced" rule is rexdd_rule_EL;
     *      4. Incoming edge rule is rexdd_rule_EH, edge "*reduced" rule is rexdd_rule_EH.
     *
     *      The edge "*out" is set to "*reduced".
     *
     *      Incoming edge rule is not rexdd_rule_X, edge "*reduced" rule is rexdd_rule_X (skip 0 node).
     *      The edge "*out" rule is set to be the incoming edge rule, while the swap bit, complement bit,
     *      target are set to be edge "*reduced"'s.
     * --------------------------------------------------------------------------------------------*/
    if ((l.rule != rexdd_rule_X)
                && (reduced->label.rule == rexdd_rule_X) && (reduced_skip == 0)) {
        rexdd_set_edge(out,
                l.rule,
                reduced->label.complemented,
                reduced->label.swapped,
                reduced->target);
    } else if (((l.rule == rexdd_rule_X) && (incoming_skip == 0))
        ||
        ((l.rule == rexdd_rule_X) && (incoming_skip >= 1) && (reduced->label.rule == rexdd_rule_X))
        ||
        (rexdd_is_EL(l.rule) && rexdd_is_EL(reduced->label.rule)
            && rexdd_is_one(l.rule) == rexdd_is_one(reduced->label.rule))
        ||
        (rexdd_is_EH(l.rule) && rexdd_is_EH(reduced->label.rule)
            && rexdd_is_one(l.rule) == rexdd_is_one(reduced->label.rule))) {
        rexdd_set_edge(out,
                reduced->label.rule,
                reduced->label.complemented,
                reduced->label.swapped,
                reduced->target);
    /* ---------------------------------------------------------------------------------------------
     * Incompatible merge and push up one
     *
     *      1. Incoming edge rule is rexdd_rule_X (skip 1 or more nodes), the edge "*reduced"
     *         rule is not rexdd_rule_X. A new node upon the target node level of "*reduced"
     *         will be created, its both edges to the target node of "*reduced".
     *      2. Incoming edge rule is rexdd_rule_EL, the edge "*reduced" rule is not rexdd_rule_EL.
     *         A new node upon the target node level of "*reduced" will be created, its Low edge
     *         to the terminal 0 while High edge to the tartget node of "*reduced".
     *      3. Incoming edge rule is rexdd_rule_EH, the edge "*reduced" rule is not rexdd_rule_EH.
     *         A new node upon the target node level of "*reduced" will be created, its High edge
     *         to the terminal 0 while Low edge to the tartget node of "*reduced".
     *
     *      The new node level is set to n+1.
     * --------------------------------------------------------------------------------------------*/
    } else if (((l.rule == rexdd_rule_X) && (incoming_skip >= 1) && (reduced->label.rule != rexdd_rule_X))){
        // push up one
        rexdd_unpacked_node_t new_p;
        new_p.edge[0] = *reduced;
        new_p.edge[1] = *reduced;
        new_p.level = n+1;
        rexdd_normalize_node(&new_p, out);
        rexdd_node_handle_t new_handle = rexdd_nodeman_get_handle(F->M, &new_p);
        new_handle = rexdd_insert_UT(F->UT, new_handle);
        out->label.rule = rexdd_rule_X;
        out->target = new_handle;
    } else if ((rexdd_is_EL(l.rule) && !rexdd_is_EL(reduced->label.rule))) {
        // push up one
        rexdd_unpacked_node_t new_p;
        new_p.edge[0].target = rexdd_make_terminal(0);
        new_p.edge[0].label.rule = rexdd_rule_X;
        new_p.edge[0].label.complemented = rexdd_is_one(l.rule);
        new_p.edge[0].label.swapped = 0;
        new_p.edge[1] = *reduced;
        new_p.level = n+1;
        rexdd_normalize_node(&new_p, out);
        rexdd_node_handle_t new_handle = rexdd_nodeman_get_handle(F->M, &new_p);
        new_handle = rexdd_insert_UT(F->UT, new_handle);
        if (incoming_skip > 1) {
            out->label.rule = l.rule;
        } else {
            out->label.rule = rexdd_rule_X;
        }
        out->target = new_handle;
    } else if ((rexdd_is_EH(l.rule) && !rexdd_is_EH(reduced->label.rule))) {
        // push up one
        rexdd_unpacked_node_t new_p;
        new_p.edge[0] = *reduced;
        new_p.edge[1].target = rexdd_make_terminal(0);
        new_p.edge[1].label.rule = rexdd_rule_X;
        new_p.edge[1].label.complemented = rexdd_is_one(l.rule);
        new_p.edge[1].label.swapped = 0;
        new_p.level = n+1;
        rexdd_normalize_node(&new_p, out);
        rexdd_node_handle_t new_handle = rexdd_nodeman_get_handle(F->M, &new_p);
        new_handle = rexdd_insert_UT(F->UT, new_handle);
        if (incoming_skip > 1) {
            out->label.rule = l.rule;
        } else {
            out->label.rule = rexdd_rule_X;
        }
        out->target = new_handle;
    /* ---------------------------------------------------------------------------------------------
     * Incompatible merge and push up all
     *
     *      1. Incoming edge rule is rexdd_rule_ALt, the edge "*reduced" <rho, s, c, q> rule is
     *         rexdd_rule_X. There will be (m - p.level) new nodes p_i created from level p.level+1
     *         to m (1 <= i <= m - p.level). Node p_1 Low edge to terminal 0 with rule rexdd_rule_X,
     *         swap bit 0, complement bit t; while High edge to node q with rule rexdd_rule_X, swap
     *         bit s, complement bit c. (p.level = n)
     *
     *              Node p_k at level n+k Low edge to node p_(k-1) with rule rexdd_rule_X, swap bit 0,
     *              complement bit 0; while High edge to node q with rule rexdd_rule_X, swap bit s,
     *              complement bit c (k>1).
     *
     *      2. Incoming edge rule is rexdd_rule_AHt, the edge "*reduced" <rho, s, c, q> rule is
     *         rexdd_rule_X. There will be (m - p.level) new nodes p_i created from level p.level+1
     *         to m (1 <= i <= m - p.level). Node p_1 High edge to terminal 0 with rule rexdd_rule_X,
     *         swap bit 0, complement bit t; while Low edge to node q with rule rexdd_rule_X, swap
     *         bit s, complement bit c. (p.level = n)
     *
     *              Node p_k at level n+k High edge to node p_(k-1) with rule rexdd_rule_X, swap bit 0,
     *              complement bit 0; while Low edge to node q with rule rexdd_rule_X, swap bit s,
     *              complement bit c (k>1).
     *
     *      3. Incoming edge rule is rexdd_rule_ALt, the edge "*reduced" <rho, s, c, q> rule is not
     *         rexdd_rule_X. There will be (m - p.level + 1) new nodes p_i created from level p.level+1
     *         to m (1 <= i <= m - p.level + 1). Node p_1 and p_2 are both at level p.level+1. p_1 with
     *         both child edges same as the edge "*reduced" <rho, s, c, q>; while p_2 Low edge to terminal
     *         0 with rule rexdd_rule_X, swap bit 0 and complement bit t, High edge same as the edge
     *         "*reduced" <rho, s, c, q>. (p.level = n)
     *
     *              Node p_k at level n+k-1 Low edge to node p_(k-1) with rule rexdd_rule_X, swap bit 0,
     *              complement bit 0; while High edge to node p_1 with rule rexdd_rule_X, swap bit 0,
     *              complement bit 0 (k>2).
     *
     *      4. Incoming edge rule is rexdd_rule_AHt, the edge "*reduced" <rho, s, c, q> rule is not
     *         rexdd_rule_X. There will be (m - p.level + 1) new nodes p_i created from level p.level+1
     *         to m (1 <= i <= m - p.level + 1). Node p_1 and p_2 are both at level p.level+1. p_1 with
     *         both child edges same as the edge "*reduced" <rho, s, c, q>; while p_2 High edge to terminal
     *         0 with rule rexdd_rule_X, swap bit 0 and complement bit t, Low edge same as the edge
     *         "*reduced" <rho, s, c, q>. (p.level = n)
     *
     *              Node p_k at level n+k-1 High edge to node p_(k-1) with rule rexdd_rule_X, swap bit 0,
     *              complement bit 0; while Low edge to node p_1 with rule rexdd_rule_X, swap bit 0,
     *              complement bit 0 (k>2).
     * --------------------------------------------------------------------------------------------*/
    } else if ((rexdd_is_AL(l.rule) && reduced->label.rule == rexdd_rule_X && reduced_skip>0)
                ||
                (rexdd_is_AH(l.rule) && reduced->label.rule == rexdd_rule_X && reduced_skip>0)) {
        // Push up all
        bool flag = rexdd_is_AH(l.rule);

        /* Build the node at +1 level and normalize it    */
        rexdd_unpacked_node_t new_p;
        new_p.level = n+1;
        new_p.edge[!flag] = *reduced;
        rexdd_set_edge(&new_p.edge[flag],
                rexdd_rule_X,
                rexdd_is_one(l.rule),
                0,
                rexdd_make_terminal(0));     // terminal node

        rexdd_edge_t temp;
        temp.label.rule = rexdd_rule_X;
        temp.target = rexdd_make_terminal(0);
        rexdd_normalize_node(&new_p, &temp);
        
        rexdd_node_handle_t handle_new_p = rexdd_nodeman_get_handle(F->M, &new_p);
        handle_new_p = rexdd_insert_UT(F->UT, handle_new_p);
        temp.target = handle_new_p;

        /* Repeat building the nodes up */
        for (uint32_t i=n+2; i<=m; i++) {
            new_p.level = i;
            new_p.edge[!flag] = *reduced;
            new_p.edge[flag] = temp;
            
            rexdd_normalize_node(&new_p, &temp);
            handle_new_p = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, &new_p));
            temp.target = handle_new_p;
        }
        out->label = temp.label;
        out->target = temp.target;

    } else if ((rexdd_is_AL(l.rule) && reduced->label.rule != rexdd_rule_X)
                ||
                (rexdd_is_AH(l.rule) && reduced->label.rule != rexdd_rule_X)) {
        // Push up all
        bool flag = rexdd_is_AH(l.rule);

        /* Build the node with same child reduced edges and normalize it    */
        rexdd_unpacked_node_t new_p;
        new_p.level = n+1;
        new_p.edge[0] = *reduced;
        new_p.edge[1] = *reduced;
        rexdd_edge_t temp1;
        temp1.label.rule = rexdd_rule_X;
        temp1.target = rexdd_make_terminal(0);
        rexdd_normalize_node(&new_p, &temp1);

        rexdd_node_handle_t handle_new_p1 = rexdd_nodeman_get_handle(F->M, &new_p);
        handle_new_p1 = rexdd_insert_UT(F->UT, handle_new_p1);
        temp1.target = handle_new_p1;

        /* Build the unpacked node at the +1 level with one edge to terminal, 
           it may be reduced or not. So we call rexdd_reduce_node           */
        rexdd_set_edge(&new_p.edge[flag],
                rexdd_rule_X,
                rexdd_is_one(l.rule),
                0,
                rexdd_make_terminal(0));     // terminal node
        new_p.edge[!flag] = *reduced;
        rexdd_edge_t temp;
        temp.label.rule = rexdd_rule_X;
        temp.target = rexdd_make_terminal(0);

        rexdd_reduce_node(F,&new_p, &temp); 
        rexdd_node_handle_t handle_new_p = temp.target;

        /* Repeat building the nodes up */
        for (uint32_t i=n+2; i<=m; i++) {
            new_p.level = i;
            new_p.edge[flag] = temp;
            new_p.edge[!flag] = temp1;
            temp.label.rule = rexdd_rule_X;

            rexdd_normalize_node(&new_p, &temp);
            handle_new_p = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, &new_p));
            temp.target = handle_new_p;
        }
        out->label = temp.label;
        out->target = temp.target;

    }
}

/* ================================================================================================ */
void rexdd_reduce_edge(
        rexdd_forest_t          *F,
        uint_fast32_t           m,
        rexdd_edge_label_t      l,
        rexdd_unpacked_node_t   p,
        rexdd_edge_t            *out)
{
    rexdd_sanity1(m >= p.level, "Bad incoming edge level");
    rexdd_sanity1(out, "null reduced edge");

    if (p.level == 0) {
        if (l.rule != rexdd_rule_X && rexdd_is_one(l.rule) == l.complemented) {
            rexdd_set_edge(out,
                    rexdd_rule_X,
                    l.complemented,
                    0,
                    rexdd_make_terminal(0));
        } else if (rexdd_is_AL(l.rule) && l.complemented && m>1) {
            rexdd_set_edge(out,
                    rexdd_rule_EH1,
                    0,
                    0,
                    rexdd_make_terminal(0));
        } else if (rexdd_is_AL(l.rule) && !l.complemented && m>1) {
            rexdd_set_edge(out,
                    rexdd_rule_EH0,
                    1,
                    0,
                    rexdd_make_terminal(0));
        } else if ((rexdd_is_AH(l.rule) && l.complemented && m>1)
                    ||
                    (rexdd_is_AL(l.rule) && !l.complemented && m==1)
                    ||
                    ((rexdd_is_AH(l.rule) || rexdd_is_EH(l.rule)) && l.complemented && m==1)) {
            rexdd_set_edge(out,
                    rexdd_rule_EL1,
                    0,
                    0,
                    rexdd_make_terminal(0));
        } else if ((rexdd_is_AH(l.rule) && !l.complemented && m>1)
                    ||
                    (rexdd_is_AL(l.rule) && l.complemented && m==1)
                    ||
                    ((rexdd_is_AH(l.rule) || rexdd_is_EH(l.rule)) && !l.complemented && m==1)) {
            rexdd_set_edge(out,
                    rexdd_rule_EL0,
                    1,
                    0,
                    rexdd_make_terminal(0));
        } else {
            rexdd_set_edge(out,
                    l.rule,
                    l.complemented,
                    0,
                    rexdd_make_terminal(0));
        }
    } else {
        if (l.swapped) {
            rexdd_node_sw(&p);
            l.swapped = 0;
        }

        if (l.complemented) {
            rexdd_edge_com(&p.edge[0]);
            rexdd_edge_com(&p.edge[1]);
            l.complemented = 0;
        }

        rexdd_edge_t reduced;
        clock_t start, end;
        start = clock();
        rexdd_reduce_node(F, &p, &reduced);
        end = clock();
        printf("** reduc_node time: %d clocks **\n", (int)(end-start));
        rexdd_merge_edge(F, m, p.level, l, &reduced, out);
    }
}

/* ================================================================================================ */

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
            result = e->label.complemented ^ rexdd_terminal_value(e->target);
        } else {
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
                if ((rexdd_is_EL(e->label.rule) && vars_and)
                    || (rexdd_is_AH(e->label.rule) && !vars_and)
                    || (rexdd_is_AL(e->label.rule) && vars_or)
                    || (rexdd_is_EH(e->label.rule) && !vars_or)) {
                    result = rexdd_eval(F, e, k, vars);
                } else {
                    result = rexdd_is_one(e->label.rule);
                }
            }
    }

    return result;
}

