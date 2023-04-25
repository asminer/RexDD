#ifndef COMPT_H
#define COMPT_H

/*
 *  Comoputing table.
 */
#include "nodeman.h"

/*
 *  malloc when insert, first64 0...49 will tell the next pointer in table
 */
typedef struct
{
    rexdd_packed_node_t *node;
    rexdd_edge_t *edge;
} rexdd_edge_in_ct;

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
    rexdd_edge_in_ct* table;
    uint_fast32_t* handles;
    /*
     *  More like an array for indexes of front
     *  hash -> index of handles -> index of table
     */ 
    uint_fast32_t first_unalloc;
    uint_fast32_t free_list;
    uint_fast32_t num_entries;

    uint_fast32_t size;
    uint_fast32_t enlarge;
    unsigned size_index;        // index of "primes" for enlarge
    unsigned prev_size_index;

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
 *  Check entry in a computing table.
 *  If cached, returns 1 and corresponding edge is set to *e;
 *  otherwise, returns 0 and *e is not changed
 */
char rexdd_check_CT(rexdd_comp_table_t *CT, rexdd_unpacked_node_t *node, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Insert a unpacked node and corresponding edge *e into computing table
 */
void rexdd_insert_CT(rexdd_comp_table_t *CT, rexdd_unpacked_node_t *node, rexdd_edge_t *e);

/****************************************************************************
 *
 *  Remove all entries that edge targets to unmarked node from the computing table
 *  nodeman *M is used to check if marked
 */
void rexdd_sweep_CT(rexdd_comp_table_t *CT, rexdd_nodeman_t *M);

#endif