#ifndef COMPT_H
#define COMPT_H

/*
 *  Comoputing table.
 */
#include "nodeman.h"

/*
 *  Entry for AND computing table
 *  malloc when insert
 */
typedef struct
{
    uint32_t lvl;
    rexdd_edge_t *edge1;
    rexdd_edge_t *edge2;
    rexdd_edge_t *edgeA;
} rexdd_edge_in_ct;

/****************************************************************************
 *
 *  Computing (hash) table of rexdd edges.
 *
 *  The computing table (CT) grows dynamically
 *  with the number of elements in the table, 
 *  according to a fixed sequence of prime sizes.
 *
 */
typedef struct 
{
    rexdd_edge_in_ct* table;       // initialized if type is 'A', otherwise, 0.

    uint_fast64_t num_entries;      // the number of valid entries
    int size_index;                 // index of primes for table size
} rexdd_comp_table_t;

/****************************************************************************
 *
 *  Initialize a computing table
 *
 */
void rexdd_init_CT(rexdd_comp_table_t *CT);

/****************************************************************************
 *
 *  Free memory for a computing table.
 */
void rexdd_free_CT(rexdd_comp_table_t *CT);

/****************************************************************************
 *
 *  Check entry in a computing table for AND operation.
 *  If cached, returns 1 and corresponding edge is set to *e;
 *  otherwise, returns 0 and *e is not changed
 */
char rexdd_check_CT(rexdd_comp_table_t *CT, uint32_t n, rexdd_edge_t *edge1, rexdd_edge_t *edge2, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Insert a unpacked node and corresponding edge *e into 
 *  computing table for AND operation
 */
void rexdd_cache_CT(rexdd_comp_table_t *CT, uint32_t n, rexdd_edge_t *edge1, rexdd_edge_t *edge2, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Remove all entries that contain unmarked node from the computing table
 *  nodeman *M is used to check if marked
 */
void rexdd_sweep_CT(rexdd_comp_table_t *CT, rexdd_nodeman_t *M);

/****************************************************************************
 *  Hash a pair of edges, modulo m
 * 
 */
static inline uint_fast64_t 
rexdd_hash_edges(const uint32_t lvl, const rexdd_edge_t* edge1, const rexdd_edge_t* edge2, uint_fast64_t m)
{
    uint64_t h;
    if (0 == (m & ~((0x01ul << 32) - 1)) ) {
        /*
         *  m fits in 32 bits
         */
        h = (edge1->target & edge2->target) % m;
        h = ((h << 32) | 
            (edge1->label.rule << 27) | 
            (edge2->label.rule << 22)) % m;
        h = ((h << 32) | 
            (edge1->label.complemented << 24) | 
            (edge2->label.complemented << 16) |
            (edge1->label.swapped << 8) | 
            (edge2->label.swapped)) % m;
        return ((h << 32) | lvl) % m;
    } 

    if (0 == (m & ~ ((0x01ul << 16) - 1)) ) {
        /*
         *  m fits in 48 bits
         */
        h = (edge1->target & edge2->target) % m;
        h = ((h << 16) | 
            (edge1->label.rule << 11) | 
            (edge2->label.rule << 6)) % m;
        h = ((h << 16) | 
            (edge1->label.complemented << 12) | 
            (edge2->label.complemented << 8) |
            (edge1->label.swapped << 4) | 
            (edge2->label.swapped)) % m;
        h = ( ( h << 16) | ( lvl >> 16) ) % m;
        h = ( ( h << 16) | ( lvl & 0xffff) ) % m;
        return h;
    }

    /*
     *  m fits in 56 bits
     */
    h = (edge1->target & edge2->target) % m;
    h = ((h << 8) | 
        (edge1->label.rule << 3) | 
        (edge1->label.complemented << 2) | 
        (edge1->label.swapped << 1)) % m;
    h = ((h << 8) | 
        (edge2->label.rule << 3) | 
        (edge2->label.complemented << 2) | 
        (edge2->label.swapped << 1)) % m;
    h = ( ( h << 8 ) | ( lvl >> 24) ) % m;
    h = ( ( h << 8 ) | (( lvl >> 16) & 0xff) ) % m;
    h = ( ( h << 8 ) | (( lvl >>  8) & 0xff) ) % m;
    h = ( ( h << 8 ) | ( lvl & 0xff) ) % m;
    return h;
}

#endif