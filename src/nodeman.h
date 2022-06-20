#ifndef NODEMAN_H
#define NODEMAN_H

#include "nodepage.h"
#include "error.h"

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
typedef uint_fast64_t  rexdd_node_handle_t;


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

    /* Last recycled handle, or 0. */
    rexdd_node_handle_t previous_handle;

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


/****************************************************************************
 *
 *  Get a node handle, and fill it with an unpacked node.
 *      @param  M           Node manager
 *      @param  n           Unpacked node
 *
 *      @return A node handle that now stores n.
 *
 */
rexdd_node_handle_t
rexdd_nodeman_get_handle(rexdd_nodeman_p M, const rexdd_unpacked_node_p n);


/****************************************************************************
 *
 *  Recycle the node handle (and node) just returned by
 *  rexdd_nodeman_get_handle().  We can only have one recycled handle at a time.
 *
 *      @param  M           Node manager
 *      @param  h           Node handle to recycle
 *
 */
static inline
void rexdd_nodeman_reuse(rexdd_nodeman_p M, rexdd_node_handle_t h)
{
    rexdd_sanity1(M, "Null node manager");
    rexdd_sanity1(0==M->previous_handle, "Too many reused node manager handles");

    M->previous_handle = h;
}


/****************************************************************************
 *
 *  Fill in an unpacked node from a node handle.
 *
 *      @param  M           Node manager
 *      @param  h           Node handle
 *      @param  u           Unpacked node to fill
 *
 */
static inline
void rexdd_unpack_handle(const rexdd_nodeman_p M, rexdd_node_handle_t h,
        rexdd_unpacked_node_p u)
{
    rexdd_sanity1(M, "Null node manager");
    rexdd_sanity1(M->pages, "Empty node manager");
    rexdd_sanity1(h, "Null handle in unpack");

    --h;
    const uint_fast32_t slot = h & 0x00ffffff;
    const uint_fast32_t pnum = h >> 24;

    rexdd_sanity2(pnum < M->max_pages, "Bad page %lu in handle", pnum);
    rexdd_packed_to_unpacked(M->pages[pnum].chunk+slot, u);
}

//
// OLD BELOW HERE
//



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
