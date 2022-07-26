
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

    // Low child node address and swapped bit < High child node address and swapped bit
    if ((P->edge[0].target < P->edge[1].target)
        || ((P->edge[0].target == P->edge[1].target)
            && (P->edge[0].label.swapped < P->edge[1].label.swapped))) {
        if (P->edge[0].label.complemented){
            out->label.complemented = 1;
            rexdd_edge_com (&(P->edge[0]));
            rexdd_edge_com (&(P->edge[1]));
        }

    // Low child node address and swapped bit > High child node address and swapped bit
    } else if ((P->edge[0].target > P->edge[1].target)
                || ((P->edge[0].target == P->edge[1].target)
                    && (P->edge[0].label.swapped > P->edge[1].label.swapped))) {
        if (P->edge[1].label.complemented){
            out->label.complemented = 1;
            out->label.swapped = 1;
            rexdd_edge_com (&(P->edge[0]));
            rexdd_edge_com (&(P->edge[1]));
            rexdd_node_sw (P);
        } else {
            out->label.swapped = 1;
            rexdd_node_sw (P);
        }

    // Low child node address and swapped bit = High child node address and swapped bit
    } else {
        if (!P->edge[0].label.complemented && !P->edge[1].label.complemented) {
            if (P->edge[0].label.rule > P->edge[1].label.rule) {
                out->label.swapped = 1;
                rexdd_node_sw (P);
            }
        } else if (P->edge[0].label.complemented && P->edge[1].label.complemented) {
            if (rexdd_rule_com_t(P->edge[0].label.rule) <= rexdd_rule_com_t(P->edge[1].label.rule)) {
                out->label.complemented = 1;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
            } else {
                out->label.complemented = 1;
                out->label.swapped = 1;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
                rexdd_node_sw (P);
            }
        } else if (!P->edge[0].label.complemented && P->edge[1].label.complemented) {
            if (P->edge[0].label.rule > rexdd_rule_com_t(P->edge[1].label.rule)) {
                out->label.complemented = 1;
                out->label.swapped = 1;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
                rexdd_node_sw (P);
            }
        } else {
            if (rexdd_rule_com_t(P->edge[0].label.rule) <= P->edge[1].label.rule) {
                out->label.complemented = 1;
                rexdd_edge_com (&(P->edge[0]));
                rexdd_edge_com (&(P->edge[1]));
            } else {
                out->label.swapped = 1;
                rexdd_node_sw (P);
            }
        }
    }
}

/* ================================================================================================ */

void rexdd_reduce_node(
        rexdd_forest_t          *F,
        rexdd_unpacked_node_t   *P,
        rexdd_edge_t            *reduced)
{
    rexdd_sanity1(reduced, "null reduced esge");

    // The level difference between unpacked node and both child nodes
    uint_fast32_t ln, hn;

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
            P->edge[0].label.rule = rexdd_rule_X;
        }

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
                            && (rexdd_is_EH(P->edge[1].label.rule)
                                || 
                                (rexdd_is_AH(P->edge[1].label.rule) && hn>2)))
                        )
                    ) {

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
                            && (rexdd_is_EL(P->edge[0].label.rule)
                                || 
                                (rexdd_is_AL(P->edge[0].label.rule) && ln>2)))
                        )
                    ) {

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
        */
        } else {
            // rexdd_normalize_node will set edge "*reduced" swap bit and complement bit
            rexdd_normalize_node(P, reduced);
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
                (rexdd_is_EL(P->edge[1].label.rule) && hn>1))) {
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
            rexdd_normalize_node(P, reduced);
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
                (rexdd_is_EH(P->edge[0].label.rule) && ln>1))) {
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
            rexdd_normalize_node(P, reduced);
            reduced->label.rule = rexdd_rule_X;
            rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
            handle = rexdd_insert_UT(F->UT, handle);
            reduced->target = handle;
        }
    }

    /* ---------------------------------------------------------------------------------------------
     * Forbidden patterns of nodes with both edges to nonterminal in canonical RexBDDs
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
                        ||(rexdd_is_AH(P->edge[1].label.rule) && hn>2))) {
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
            rexdd_normalize_node(P, reduced);
            reduced->label.rule = rexdd_rule_X;
            rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F->M, P);
            handle = rexdd_insert_UT(F->UT, handle);
            reduced->target = handle;
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
        rexdd_reduce_node(F, new_p, reduced);
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

