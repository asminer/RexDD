#ifndef UNPACKED_H
#define UNPACKED_H

#include <stdint.h>
#include <stdbool.h>

/*
 *  Reduction rules (TBD):
 *      N, X, LZ, LN, HZ, HN, ELZ, ELN, EHZ, EHN, ALZ, ALN, AHZ, AHN
*/
typedef enum {
    N = 0,
    X = 1,
    LZ = 2,
    LN = 3,
    HZ = 4,
    HN = 5,
    ELZ = 6,
    ELN = 7,
    EHZ = 8,
    EHN = 9,
    ALZ = 10,
    ALN = 11,
    AHZ = 12,
    AHN = 13
} rexdd_rule_t;

/*
 *  The complement of reduction rules
 */
static inline rexdd_rule_t rexdd_rule_com_t (rexdd_rule_t R) {
    switch (R)
    {
    case N:
        return N;

    case X:
        return X;

    case LZ:
        return LN;

    case LN:
        return LZ;

    case HZ:
        return HN;

    case HN:
        return HZ;

    case ELZ:
        return ELN;

    case ELN:
        return ELZ;

    case EHZ:
        return EHN;

    case EHN:
        return EHZ;

    case ALZ:
        return ALN;

    case ALN:
        return ALZ;

    case AHZ:
        return AHN;

    case AHN:
        return AHZ;

    default:
        return N;
    }

}


static const char* rexdd_rule_name[] = {
    "N",
    "X",
    "L0",
    "L1",
    "H0",
    "H1",
    "EL0",
    "EL1",
    "EH0",
    "EH1",
    "AL0",
    "AL1",
    "AH0",
    "AH1",
    "?",
    "?",
//
    "IN",
    "IX",
    "IL0",
    "IL1",
    "IH0",
    "IH1",
    "IEL0",
    "IEL1",
    "IEH0",
    "IEH1",
    "IAL0",
    "IAL1",
    "IAH0",
    "IAH1",
    "I?",
    "I?",
};

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

typedef rexdd_unpacked_node_t* rexdd_unpacked_node_p;

/*
 *  The complement of rexdd edge
 */
static inline void rexdd_edge_com (rexdd_edge_t e) {
    e.label.rule = rexdd_rule_com_t (e.label.rule);
    e.label.complemented = ~e.label.complemented;
}

#endif
