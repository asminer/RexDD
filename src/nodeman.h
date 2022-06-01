#ifndef NODEMAN_H
#define NODEMAN_H

#include "unpacked.h"

/*
 *  TBD - overview of these functions
 *          (these are all expert level or higher)
 */

/*
 * Node handle.
 */
typedef uint64  rexdd_node_handle;


typedef struct rexdd_nodeman* rexdd_nodeman_p;

/*
 *  Initialize a node manager.
 *
 *  TBD - what settings here?
 *      maybe max number of levels?
 *      maybe max number of nodes?
 *
 *  return 0 on success...
 */
int rexdd_create_nodeman(rexdd_nodeman_p M);

/*
 *  Destroy and free memory used by a node manager.
 */
int rexdd_destroy_nodeman(rexdd_nodeman_p M);


/*
 *  Fill in an unpacked node from a node handle.
 *
 *  return 0 on success...
 */
int rexdd_fill_unpacked(const rexdd_nodeman_p M, rexdd_node_handle n,
        rexdd_unpacked_node_p u);



#endif
