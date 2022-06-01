#ifndef NODEMAN_H
#define NODEMAN_H

#include "unpacked.h"

/*
 *  TBD - overview of these functions
 *          (these are all expert level or higher)
 */

#define REXDD_PAGESIZE (1024*1024)

/*
 * Node handle.
 */
typedef uint_fast64_t  rexdd_node_handle;

/*
 * TBD - design this struct
 */
struct rexdd_nodepage {
    /*  Array for node storage. */
    /*  first unallocated */
    /*  list of free slots */
    /*  total number of free slots */
    /*  pointer (index) for various lists of pages */
};

/*
 *  TBD - design this struct
 *      should be visible b/c it will be needed for macros below.
 */
struct rexdd_nodeman {
    /* Array of pointers to rexdd_nodepage  */
    /* array size */
    /* last used in the array */
    /* list of pages with free slots, in increasing order of #free slots */
};

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
 *  TBD - compaction?
 *      need a renumbering array
 */

/*
 *  Get a free node handle
 */
rexdd_node_handle   rexdd_new_handle(rexdd_nodeman_p M);

/*
 *  Return a node handle to the free store
 */
void    rexdd_free_handle(rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Fill in an unpacked node from a node handle.
 *
 *  return 0 on success...
 */
int rexdd_unpack_handle(rexdd_unpacked_node_p u,
        const rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Fill in a packed node at the specified handle.
 */
int rexdd_pack_handle(rexdd_nodeman_p M, rexdd_node_handle h,
        const rexdd_unpacked_node_p u);


/*
 *  Get the next handle in a chain.
 *      (probably as a macro or static inline function (allowed since C99))
 */
rexdd_node_handle GET_NEXT_HANDLE(const rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Set the next handle in a chain.
 *      (probably as a macro)
 */
rexdd_node_handle SET_NEXT_HANDLE(rexdd_nodeman_p M, rexdd_node_handle h,
        rexdd_node_handle nxt);

/*
 *  Is a node marked?
 *      (probably as a macro)
 */
uint64_t  IS_HANDLE_MARKED(const rexdd_nodeman_p M, rexdd_node_handle h);

/*
 *  Mark a node.
 *      (probably as a macro)
 */
void    MARK_HANDLE(rexdd_nodeman_p M, rexdd_node_handle h);

/*
 *  Unmark a node.
 *      (probably as a macro)
 */
void    UNMARK_HANDLE(rexdd_nodeman_p M, rexdd_node_handle h);

#endif
