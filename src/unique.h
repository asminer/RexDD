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
    const rexdd_nodeman_p M;    /* node manager */
    // ...
} rexdd_unique_table_t;

typedef rexdd_unique_table_t* rexdd_unique_table_p;

/*
 *  Initialize a unique table
 *
 *  return 0 on success, ...
 */
int rexdd_create_UT(rexdd_unique_table_p T, const rexdd_nodeman_p M);

/*
 *  Destroy a unique table
 */
void rexdd_destroy_UT(rexdd_unique_table_p T);

/*
 *  Add a handle to the unique table.
 *  If unique, returns the same handle;
 *  otherwise, returns the handle of the duplicate.
 */
rexdd_node_handle_t rexdd_UT_insert(rexdd_unique_table_p T, rexdd_node_handle_t h);

/*
 *  Remove a single handle from the unique table.
 */
void rexdd_UT_remove(rexdd_unique_table_p T, rexdd_node_handle_t h);


/*
 *  Remove all marked/unmarked nodes from the unique table.
 *      @param  T   Unique table to modify
 *      @param  b   if true, remove marked; if false, remove unmarked.
 */
void rexdd_UT_remove_all(rexdd_unique_table_p T, bool b);



#endif
