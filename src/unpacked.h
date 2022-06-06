#ifndef UNPACKED_H
#define UNPACKED_H

#include <stdint.h>
#include <stdbool.h>

/*
 *  Reduction rules (TBD):
 *      N, X, LZ, LN, HZ, HN, ELZ, ELN, EHZ, EHN, ALZ, ALN, AHZ, AHN
*/
enum rexdd_reduction_rule {
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
};

/*
 * Edge label info
 */
typedef struct {
    enum rexdd_reduction_rule rule;
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
    rexdd_edge_t  edge;
} rexdd_unpacked_node_t;

typedef rexdd_unpacked_node_t* rexdd_unpacked_node_p;

#endif