#ifndef OPERATION_H
#define OPERATION_H

#include "forest.h"
#include "helpers.h"

#include <assert.h>

/**
 * @brief Normalize an edge if it is a constant edge
 * 
 * @param e 
 */
static inline void rexdd_normalize_edge(rexdd_forest_t *F, rexdd_edge_t* e)
{
    if (F->S.bdd_type == CQBDD || F->S.bdd_type == CSQBDD
        || F->S.bdd_type == CFBDD || F->S.bdd_type == CSFBDD
        || F->S.bdd_type == CESRBDD || F->S.bdd_type == REXBDD) {
        // for the BDDs with complement bit
        if (rexdd_is_terminal(e->target)) {
            e->label.complemented = (rexdd_terminal_value(e->target) == 1) ^ e->label.complemented;
            e->target = rexdd_make_terminal(0);
        }
    }
// #if defined C_QBDD || defined CS_QBDD || defined C_FBDD || defined CS_FBDD || defined CESRBDD || defined REXBDD
    
// #endif
    if (F->S.bdd_type == SQBDD || F->S.bdd_type == SFBDD || F->S.bdd_type == REXBDD) {
        if (rexdd_is_terminal(e->target)) {
            e->label.swapped = 0;
        }
    }
// #if defined S_QBDD || defined S_FBDD || defined REXBDD
    
// #endif
    if (F->S.bdd_type == CESRBDD || F->S.bdd_type == REXBDD) {
        if (rexdd_is_terminal(e->target) && e->label.rule != rexdd_rule_X
            && rexdd_is_one(e->label.rule) == e->label.complemented) {
            e->label.rule = rexdd_rule_X;
        }
    }
// #if defined REXBDD || defined CESRBDD
    
// #endif
    if (F->S.bdd_type == ZBDD || F->S.bdd_type == ESRBDD) {
    if (rexdd_is_terminal(e->target) && !rexdd_terminal_value(e->target)
            && e->label.rule != rexdd_rule_X) {
            e->label.rule = rexdd_rule_X;
        }
    }
// #if defined ZBDD || defined ESRBDD
    
// #endif
}

/**
 * @brief Helper function to decide which pattern is this edge
 * 
 * @param e                     Edge to decide
 * @return char                 'L' for Low, 'H' for High, 'U' for Uncertain
 */
char rexdd_edge_pattern(rexdd_edge_t* e);

/**
 * @brief Helper function to return the x-edge or y-edge of a long edge
 * 
 * @param e                     Long edge
 * @param xy                    'x'/'y' or 'X'/'Y' edge
 * @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_expand_edgeXY(rexdd_edge_t* e, char xy);


/**
 *  Build Low pattern
 *  
 * 
 * @param F                     Forest containing BDDs and result
 * @param ex                    Left side edge of Low pattern
 * @param ey                    Right side edge of Low pattern
 * @param n                     Level of the starting nonterminal node
 * @param m                     Level of the ending nonterminal nodes
 * @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_build_L(rexdd_forest_t* F, rexdd_edge_t* ex, rexdd_edge_t* ey, uint32_t n, uint32_t m);

/**
 *  Build High pattern
 *  
 * 
 * @param F                     Forest containing BDDs and result
 * @param ex                    Left side edge of High pattern
 * @param ey                    Right side edge of High pattern
 * @param n                     Level of the starting nonterminal node
 * @param m                     Level of the ending nonterminal nodes
 * @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_build_H(rexdd_forest_t* F, rexdd_edge_t* ex, rexdd_edge_t* ey, uint32_t n, uint32_t m);

/**
 *  Build Umbrella pattern
 *  
 * 
 * @param F                     Forest containing BDDs and result
 * @param ex                    Left side edge of Umbrella pattern
 * @param ey                    Middle side edge of Umbrella pattern
 * @param ez                    Right side edge of Umbrella pattern
 * @param n                     Level of the starting nonterminal node
 * @param m                     Level of the ending nonterminal nodes
 * @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_build_U(rexdd_forest_t* F, rexdd_edge_t* ex, rexdd_edge_t* ey, rexdd_edge_t* ez, uint32_t n, uint32_t m);

rexdd_edge_t rexdd_AND_LL(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2, uint32_t n);
rexdd_edge_t rexdd_AND_HH(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2, uint32_t n);
rexdd_edge_t rexdd_AND_LH(rexdd_forest_t* F, rexdd_edge_t* e1, rexdd_edge_t* e2, uint32_t n);

/**
 *  AND opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_AND_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

/**
 *  NOT operation of one edge representing one reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e                Operand edge
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_NOT_edge(rexdd_forest_t* F, const rexdd_edge_t* e, uint32_t lvl);

/**
 *  OR opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_OR_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

/**
 *  XOR opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_XOR_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

/**
 *  NAND opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_NAND_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

/**
 *  NOR opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_NOR_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

/**
 *  IMPLIES opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_IMPLIES_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

/**
 *  EQUALS opeartion of two edges representing two reduced BDDs
 * 
 *      @param F                Forest containing operand BDDs and result
 *      @param e1               Operand 1
 *      @param e2               Operand 2
 *      @param lvl              Respect of level on variables
 *      @return rexdd_edge_t 
 */
rexdd_edge_t rexdd_EQUALS_edges(rexdd_forest_t* F, const rexdd_edge_t* e1, const rexdd_edge_t* e2, uint32_t lvl);

#endif