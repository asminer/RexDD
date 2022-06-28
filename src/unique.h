#ifndef UNIQUE_H
#define UNIQUE_H

/*
 *  Unique table.
 */

#include "nodeman.h"

/*
 * TBD - design this struct
 */
typedef struct {
    rexdd_nodeman_p M;      /* node manager */
    uint_fast64_t* table;
    uint_fast64_t size;
    uint_fast64_t num_entries;
    unsigned size_index;
    unsigned prev_size_index;
} rexdd_unique_table_t;

typedef rexdd_unique_table_t* rexdd_unique_table_p;

/*
 *  Initialize a unique table
 *
 *  return 0 on success, ...
 */
int rexdd_create_UT(rexdd_unique_table_p T, rexdd_nodeman_p M);

/*
 *  Destroy a unique table
 */
void rexdd_destroy_UT(rexdd_unique_table_p T);

/*
 *  Add a handle to the unique table.
 *  If unique, returns the same handle;
 *  otherwise, returns the handle of the duplicate.
 */
rexdd_node_handle_t rexdd_insert_UT(rexdd_unique_table_p T, rexdd_node_handle_t h);

/*
 *  Remove all unmarked nodes from the unique table.
 *      @param  T   Unique table to modify
 */
void rexdd_sweep_UT(rexdd_unique_table_p T);



#endif
