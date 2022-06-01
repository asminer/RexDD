#ifndef UNPACKED_H
#define UNPACKED_H

#include <stdint.h>

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
 * "Unpacked" node
 */
struct rexdd_unpacked_node {
    uint64_t child[2];
    uint32_t level;
    uint8_t  edgerule[2];
    uint8_t  complement[2];
    uint8_t  swap[2];
};

typedef struct rexdd_unpacked_node* rexdd_unpacked_node_p;


#endif
