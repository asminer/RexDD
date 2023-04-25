#ifndef COMPT_H
#define COMPT_H

/*
 *  Comoputing table.
 */
#include "nodeman.h"

/*
 *  Entry for Reducing node computing table
 *  malloc when insert, first64 0...49 will tell the next pointer in table
 */
typedef struct
{
    rexdd_packed_node_t *node;
    rexdd_edge_t *edge;
} rexdd_reduce_edge_in_ct;


/*
 *  Entry for AND computing table
 *  malloc when insert
 */
typedef struct
{
    rexdd_edge_t *edge1;
    rexdd_edge_t *edge2;
    rexdd_edge_t *edgeA;
    uint_fast64_t next;
} rexdd_AND_edge_in_ct;


/****************************************************************************
 *
 *  Computing (hash) table of rexdd edges.
 *
 *  Each hash table entry is a chain of node handles.
 *  The next node handle is stored in the packed node.
 *
 *  The computing table (CompT) grows and shrinks dynamically
 *  with the number of elements in the table, according
 *  to a fixed sequence of prime sizes.
 *
 */

/*
* low 24 bits of h is the slot number in the page;
* the high bits are the page number.
*/
typedef struct 
{
    rexdd_reduce_edge_in_ct* tableR;    // initialized if type is 'R', otherwise, 0.
    rexdd_AND_edge_in_ct* tableA;       // initialized if type is 'A', otherwise, 0.
    uint_fast64_t* handles;
    /*
     *  More like an array for indexes of front
     *  hash -> index of handles -> index of table
     */ 
    uint_fast64_t first_unalloc;    
    uint_fast64_t free_list;            // point to +1
    uint_fast64_t num_entries;

    uint_fast64_t size;
    uint_fast64_t enlarge;
    unsigned size_index;                // index of "primes" for enlarge
    unsigned prev_size_index;
    char type;                          // 'R' for reduce; 'A' for AND

} rexdd_comp_table_t;

/****************************************************************************
 *
 *  Initialize a computing table
 *
 */
void rexdd_init_CT(rexdd_comp_table_t *CT, char type);

/****************************************************************************
 *
 *  Free memory for a computing table.
 */
void rexdd_free_CT(rexdd_comp_table_t *CT);

/****************************************************************************
 *
 *  Check entry in a computing table for reducing node.
 *  If cached, returns 1 and corresponding edge is set to *e;
 *  otherwise, returns 0 and *e is not changed
 * 
 *  Note: complement bit of low edge should be 0 before checking, since packed 
 *  node used for hashing does not store it.
 */
char rexdd_check_reduce_CT(rexdd_comp_table_t *CT, rexdd_unpacked_node_t *node, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Insert a unpacked node and corresponding edge *e into 
 *  computing table for reducing node.
 */
void rexdd_insert_reduce_CT(rexdd_comp_table_t *CT, rexdd_unpacked_node_t *node, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Check entry in a computing table for AND operation.
 *  If cached, returns 1 and corresponding edge is set to *e;
 *  otherwise, returns 0 and *e is not changed
 */
char rexdd_check_AND_CT(rexdd_comp_table_t *CT, rexdd_edge_t *edge1, rexdd_edge_t *edge2, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Insert a unpacked node and corresponding edge *e into 
 *  computing table for AND operation
 */
void rexdd_insert_AND_CT(rexdd_comp_table_t *CT, rexdd_edge_t *edge1, rexdd_edge_t *edge2, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Remove all entries that contain unmarked node from the computing table
 *  nodeman *M is used to check if marked
 */
void rexdd_sweep_CT(rexdd_comp_table_t *CT, rexdd_nodeman_t *M);

#endif