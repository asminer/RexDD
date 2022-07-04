#ifndef UNPACKED_H
#define UNPACKED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*
 *  Reduction rules (TBD):
 *      N, X, LZ, LN, HZ, HN, ELZ, ELN, EHZ, EHN, ALZ, ALN, AHZ, AHN
*/
typedef enum {
    rexdd_rule_N = 0,
    rexdd_rule_X = 1,
    rexdd_rule_LZ = 2,
    rexdd_rule_LN = 3,
    rexdd_rule_HZ = 4,
    rexdd_rule_HN = 5,
    rexdd_rule_ELZ = 6,
    rexdd_rule_ELN = 7,
    rexdd_rule_EHZ = 8,
    rexdd_rule_EHN = 9,
    rexdd_rule_ALZ = 10,
    rexdd_rule_ALN = 11,
    rexdd_rule_AHZ = 12,
    rexdd_rule_AHN = 13
} rexdd_rule_t;

/*
 *  The complement of reduction rules
 */
static inline rexdd_rule_t rexdd_rule_com_t (rexdd_rule_t R) {
    switch (R)
    {
    case rexdd_rule_N:
        return rexdd_rule_N;

    case rexdd_rule_X:
        return rexdd_rule_X;

    case rexdd_rule_LZ:
        return rexdd_rule_LN;

    case rexdd_rule_LN:
        return rexdd_rule_LZ;

    case rexdd_rule_HZ:
        return rexdd_rule_HN;

    case rexdd_rule_HN:
        return rexdd_rule_HZ;

    case rexdd_rule_ELZ:
        return rexdd_rule_ELN;

    case rexdd_rule_ELN:
        return rexdd_rule_ELZ;

    case rexdd_rule_EHZ:
        return rexdd_rule_EHN;

    case rexdd_rule_EHN:
        return rexdd_rule_EHZ;

    case rexdd_rule_ALZ:
        return rexdd_rule_ALN;

    case rexdd_rule_ALN:
        return rexdd_rule_ALZ;

    case rexdd_rule_AHZ:
        return rexdd_rule_AHN;

    case rexdd_rule_AHN:
        return rexdd_rule_AHZ;

    default:
        return rexdd_rule_N;
    }

}


/*
 * Edge label info
 */
typedef struct {
    rexdd_rule_t rule;
    bool complemented;
    bool swapped;
} rexdd_edge_label_t;

/*
 * A complete edge
 */
typedef struct {
    rexdd_edge_label_t label;
    uint_fast64_t target;
} rexdd_edge_t;

/*
 * "Unpacked" node
 */
typedef struct {
    uint_fast32_t level;
    rexdd_edge_t  edge[2];
} rexdd_unpacked_node_t;

/*
 *  The complement of rexdd edge
 */
static inline void rexdd_edge_com (rexdd_edge_t e) {
    e.label.rule = rexdd_rule_com_t (e.label.rule);
    e.label.complemented = !e.label.complemented;
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
