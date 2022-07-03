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
    rexdd_nodepage_t *pages;

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
void rexdd_init_nodeman(rexdd_nodeman_t *M, unsigned maxpages);


/****************************************************************************
 *
 *  Destroy a node manager.
 *  All memory is deallocated.
 *
 */
void rexdd_free_nodeman(rexdd_nodeman_t *M);


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
rexdd_nodeman_get_handle(rexdd_nodeman_t *M, const rexdd_unpacked_node_t *n);


/****************************************************************************
 *
 *  Sweep a node manager.
 *  For each node in the manager, check if it is marked or not.
 *  If marked, the mark bit is cleared.
 *  If unmarked, the node is recycled.
 *      @param  M       Node manager
 *
 */
void rexdd_sweep_nodeman(rexdd_nodeman_t *M);


/****************************************************************************
 *
 *  Recycle the node handle (and node) just returned by
 *  rexdd_nodeman_get_handle().  We can only have one recycled handle at a time.
 *
 *      @param  M           Node manager
 *      @param  h           Node handle to recycle
 *
 */
static inline void
rexdd_nodeman_reuse(rexdd_nodeman_t *M, rexdd_node_handle_t h)
{
    rexdd_sanity1(M, "Null node manager");
    rexdd_sanity1(0==M->previous_handle, "Too many reused node manager handles");

    M->previous_handle = h+1;
}


/****************************************************************************
 *
 *  Find the packed node (pointer) corresponding to a node handle.
 *
 *      @param  M           Node manager
 *      @param  h           Node handle
 *
 */
static inline rexdd_packed_node_t*
rexdd_get_packed_for_handle(rexdd_nodeman_t *M, rexdd_node_handle_t h)
{
    rexdd_sanity1(M, "Null node manager");
    rexdd_sanity1(M->pages, "Empty node manager");
    rexdd_sanity1(h, "Null node handle");

    --h;

    /*
     * low 24 bits of h is the slot number in the page;
     * the high bits are the page number.
     */

    rexdd_sanity2((h>>24) < M->max_pages, "Bad page %lu in handle", (h>>24));

    return M->pages[h >> 24].chunk + (h & 0x00ffffff);
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
static inline void
rexdd_unpack_handle(rexdd_nodeman_t *M, rexdd_node_handle_t h,
        rexdd_unpacked_node_t *u)
{
    rexdd_packed_to_unpacked( rexdd_get_packed_for_handle(M, h), u );
}


/****************************************************************************
 *
 *  Mark a handle, for use by mark and sweep garbage collection.
 *  This does NOT mark the children.
 *
 *  TBD - probably should replace this with the proper version,
 *      that can handle any destination (terminal or nonterminal)
 *      and will mark the children nodes.
 *
 *  TBD - move the "proper" version to forest.h?
 *
 *      @param  M       Node manager
 *      @param  h       Node handle
 *
 *      @return     true, iff the handle was already marked.
 *
 */
static inline bool
rexdd_mark_handle(rexdd_nodeman_t *M, rexdd_node_handle_t h)
{
    rexdd_packed_node_t *node = rexdd_get_packed_for_handle(M, h);
    if (rexdd_is_packed_marked(node)) {
        return true;
    }
    rexdd_mark_packed(node);
    return false;
}


/****************************************************************************
 *
 *  Dump a node manager, in human-readable format, for debugging purposes.
 *      @param  fout        Where to dump
 *      @param  M           Node manager to dump
 *      @param  show_used   If true, display the used nodes
 *      @param  show_unused If true, display the unused nodes
 *
 */
void rexdd_dump_nodeman(FILE* fout, const rexdd_nodeman_t *M,
        bool show_used, bool show_unused);



#endif
