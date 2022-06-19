#ifndef NODEMAN_H
#define NODEMAN_H

#include "nodepage.h"

/*
 *  Efficient storage of lots of nodes, for use by a forest.
 *
 */

/*
 *  Node handle.
 *
 *  A valid node handle is between 1 and 2^49.
 *  Zero is used as a null handle.
 *
 */
typedef uint_fast64_t  rexdd_node_handle;


/****************************************************************************
 *
 *  Node manager.
 *
 *  Conceptually, this is a large collection of non-terminal nodes,
 *  stored compactly.  Nodes are referred to by a handle, which is
 *  an index between 1 and 2^49.  Zero is the special "null" handle.
 *
 *  The collection is split into pages (the nodepage struct).
 *  Mainly the node manager keeps track of the pages, as an
 *  expanding array of nodepages.
 */
typedef struct {
    /* (Expanding) Array of pages */
    rexdd_nodepage_p pages;

    /* array size */
    uint_fast32_t pages_size;

    /* max allowed array size */
    uint_fast32_t max_pages;

    /* list of pages with free slots */
    uint_fast32_t not_full_pages;

    /* list of not allocated pages */
    uint_fast32_t empty_pages;

} rexdd_nodeman_t;

typedef rexdd_nodeman_t* rexdd_nodeman_p;


/****************************************************************************
 *
 *  Initialize a node manager.
 *  Sets up a small array of pages, and initializes the page lists.
 *      @param  M           Node manager to initialize
 *      @param  maxpages    Limit the number of pages the node manager
 *                          will ever allocate.  0, and any value above
 *                          2^25, will set the limit to 2^25.
 *
 */
void rexdd_init_nodeman(rexdd_nodeman_p M, unsigned maxpages);


/****************************************************************************
 *
 *  Destroy a node manager.
 *  All memory is deallocated.
 *
 */
void rexdd_free_nodeman(rexdd_nodeman_p M);


//
// OLD BELOW HERE
//

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


/*
 *  Compute a raw hash value for a node.
 *      (inline)
 */
static inline uint64_t HASH_HANDLE(const rexdd_nodeman_p M, rexdd_node_handle h)
{
    // TBD
    return 0;
}

#endif
