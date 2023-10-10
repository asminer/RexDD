#ifndef UNPACKED_H
#define UNPACKED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "error.h"

/****************************************************************************
 *
 *  Reduction rules (TBD: update this list)
 *
 */
typedef enum {
    rexdd_rule_X =   8,
    rexdd_rule_EL0 = 0,
    rexdd_rule_EL1 = 2,
    rexdd_rule_EH0 = 4,
    rexdd_rule_EH1 = 6,
    rexdd_rule_AL0 = 1,
    rexdd_rule_AL1 = 3,
    rexdd_rule_AH0 = 5,
    rexdd_rule_AH1 = 7
} rexdd_rule_t;

/****************************************************************************
 *
 *  Helper: is the rexdd rule specific?
 *
 */
static inline bool rexdd_is_EL(rexdd_rule_t R)
{
    return (R==rexdd_rule_EL0 || R==rexdd_rule_EL1);
}

static inline bool rexdd_is_EH(rexdd_rule_t R)
{
    return (R==rexdd_rule_EH0 || R==rexdd_rule_EH1);
}

static inline bool rexdd_is_AL(rexdd_rule_t R)
{
    return (R==rexdd_rule_AL0 || R==rexdd_rule_AL1);
}

static inline bool rexdd_is_AH(rexdd_rule_t R)
{
    return (R==rexdd_rule_AH0 || R==rexdd_rule_AH1);
}

static inline bool rexdd_is_one(rexdd_rule_t R)
{
    return R & 2;
}

/****************************************************************************
 *
 *  The complement of reduction rules
 *
 *  TBD - we could implement this as an array lookup
 *
 */
static inline rexdd_rule_t rexdd_rule_com_t (rexdd_rule_t R)
{
    if (R == rexdd_rule_X) {
        return R;
    } else if (rexdd_is_one(R)) {
        return (rexdd_rule_t)(R & 13);
    } else {
        return (rexdd_rule_t)(R | 2);
    }
}


/****************************************************************************
 *
 *  Terminal or non-terminal node in a forest.
 *
 *  Currently, 50 bits; any info above that is ignored.
 *  The high bit tells if it is terminal (1) or non-terminal (0).
 *  The low 49 bits give the terminal value or non-terminal handle.
 *
 */
typedef uint64_t    rexdd_node_handle_t;

/****************************************************************************
 *
 *  Helper: is the node handle for a terminal node?
 *
 */
static inline bool rexdd_is_terminal(rexdd_node_handle_t n)
{
    return n & ((uint64_t)0x01 << 49);
}

/****************************************************************************
 *
 *  Helper: get the terminal node value
 *
 */
static inline rexdd_node_handle_t rexdd_terminal_value(rexdd_node_handle_t n)
{
    rexdd_sanity1(rexdd_is_terminal(n), "requesting terminal value for nonterminal");
    return n & (((uint64_t)0x01 << 49)-1);
}

/****************************************************************************
 *
 *  Helper: build a terminal node index
 *      (Sets bit 49, clears bits 50 and higher)
 *
 */
static inline rexdd_node_handle_t rexdd_make_terminal(rexdd_node_handle_t n)
{
    return ( n & (((uint64_t)0x01 << 49)-1) ) | ( (uint64_t)0x01 << 49 );
}

/****************************************************************************
 *
 *  Helper: get the non-terminal node data
 *
 */
static inline rexdd_node_handle_t rexdd_nonterminal_handle(rexdd_node_handle_t n)
{
    rexdd_sanity1(!rexdd_is_terminal(n), "requesting nonterminal handle for terminal");
    return n & (((uint64_t)0x01 << 49)-1);
}

/****************************************************************************
 *
 *  Helper: build a nonterminal node index
 *      (Clears bits 49 and higher)
 *
 */
static inline rexdd_node_handle_t rexdd_make_nonterminal(rexdd_node_handle_t n)
{
    return n & (((uint64_t)0x01 << 49)-1);
}


/****************************************************************************
 *
 *  Edge label info
 *
 */
typedef struct {
    rexdd_rule_t rule;
    bool complemented;
    bool swapped;
} rexdd_edge_label_t;


/****************************************************************************
 *
 *  A complete edge
 *
 */
typedef struct {
    rexdd_edge_label_t label;
    rexdd_node_handle_t target;
} rexdd_edge_t;


/****************************************************************************
 *
 *  Helper to set a complete edge
 *
 */
static inline void rexdd_set_edge(rexdd_edge_t *E,
        rexdd_rule_t rule, bool comp, bool swap, uint_fast64_t target)
{
    rexdd_sanity1(E, "null edge in rexdd_set_edge");
    E->label.rule = rule;
    E->label.complemented = comp;
    E->label.swapped = swap;
    E->target = target;
}

/****************************************************************************
 *
 *  Helper to compare edges if equal
 *
 */
static inline bool rexdd_edges_are_equal(const rexdd_edge_t *E1, const rexdd_edge_t *E2)
{
    rexdd_sanity1(E1, "null edge in rexdd_equal_edges");
    rexdd_sanity1(E2, "null edge in rexdd_equal_edges");
    if (E1->target != E2->target) return false;
    if (E1->label.rule != E2->label.rule) return false;
    if (E1->label.complemented != E2->label.complemented) return false;
    return E1->label.swapped == E2->label.swapped;
}

/****************************************************************************
 *
 *  Helper to if two edges are complemented to each other
 *
 */
static inline bool rexdd_edges_are_complement(const rexdd_edge_t *E1, const rexdd_edge_t *E2)
{
    rexdd_sanity1(E1, "null edge in rexdd_equal_edges");
    rexdd_sanity1(E2, "null edge in rexdd_equal_edges");
    if (rexdd_is_terminal(E1->target) && rexdd_is_terminal(E2->target)) {
        return ((rexdd_terminal_value(E1->target) ^ (E1->label.complemented)) 
                    != (rexdd_terminal_value(E2->target) ^ (E2->label.complemented)))
                && (E1->label.rule == rexdd_rule_com_t(E2->label.rule));
    }

    if (E1->target != E2->target) return false;
    if (E1->label.swapped != E2->label.swapped) return false;
    return (E1->label.complemented != E2->label.complemented) 
            && (E1->label.rule == rexdd_rule_com_t(E2->label.rule));
}


/****************************************************************************
 *
 *  An "unpacked" node.
 *
 */
typedef struct {
    uint_fast32_t level;
    rexdd_edge_t  edge[2];
} rexdd_unpacked_node_t;

/*
 *  The complement of rexdd edge
 *      changing the complement bit of this edge, if allowed
 */
static inline void rexdd_edge_com (rexdd_edge_t *e) {
    // this is only used for Rex and some with complement flag
    e->label.rule = rexdd_rule_com_t (e->label.rule);
    e->label.complemented = !e->label.complemented;
}

/*
 *  Swap the edges for the target node
 */
static inline void rexdd_node_sw (rexdd_unpacked_node_t *p){
    rexdd_edge_t temp = p->edge[0];
    p->edge[0] = p->edge[1];
    p->edge[1] = temp;
}

/****************************************************************************
 *
 *  Write an edge, in human-readable format, to a char buffer.
 *
 *  @param  buffer      Character buffer to write into.  Can be null.
 *  @param  len         Length of the buffer, so we don't write past the end.
 *                      Can be 0, e.g., if buffer is null.
 *  @param  e           Edge to 'display'
 *
 */
void rexdd_snprint_edge(char* buffer, unsigned len, rexdd_edge_t e);

/****************************************************************************
 *
 *  Write an edge, in human-readable format, to a FILE.
 *
 *  @param  fout        File to write.
 *  @param  e           Edge to 'display'
 *
 */
void rexdd_fprint_edge(FILE* fout, rexdd_edge_t e);


#endif
