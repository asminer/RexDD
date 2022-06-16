#ifndef NODEPAGE_H
#define NODEPAGE_H

#include "unpacked.h"
#include "packed.h"

#define REXDD_PAGESIZE (16*1024*1024)

/****************************************************************************
 *
 *  A struct for a "page" of nodes.
 *  This is a contiguous chunk of 16 million nodes, that can be pulled
 *  from for node storage in a forest.  Freed nodes are kept in a list.
 *  Within the list, pointers are array indexes plus one, so zero may
 *  be used for null.
 *
 */
typedef struct {
    /*  Array for node storage. */
    rexdd_packed_node_p chunk;

    /*  first unallocated slot */
    uint_fast32_t first_unalloc;

    /*  list of free slots */
    uint_fast32_t free_list;

    /*  total number of free slots */
    uint_fast32_t num_unused;

    /*  pointer (index) for various lists of pages */
    uint_fast32_t next;
    // ^ not sure about this one yet

} rexdd_nodepage_t;

typedef rexdd_nodepage_t* rexdd_nodepage_p;


/****************************************************************************
 *
 *  Initialize a page of nodes.
 *  The chunk is allocated, and every slot is set to "unused".
 *
 */
void rexdd_init_nodepage(rexdd_nodepage_p np);


/****************************************************************************
 *
 *  Destroy a page of nodes.
 *  The chunk is deallocated.
 *
 */
void rexdd_init_nodepage(rexdd_nodepage_p np);


/****************************************************************************
 *
 *  Get a free slot, and fill it with an unpacked node.
 *      @param  page    Page to use.  If num_unused is zero,
 *                      rexdd_error will be called.
 *      @param  node    The unpacked node to store.
 *
 *      @return The slot number used.
 *
 */
uint_fast32_t
rexdd_fill_free_page_slot(
        rexdd_nodepage_p page,
        const rexdd_unpacked_node_p node);


/****************************************************************************
 *
 *  Unpack the node stored at a given slot.
 *      @param  page    Page to use.
 *      @param  slot    Slot storing the desired node.
 *                      Will call rexdd_error if this slot is unallocated.
 *      @param  node    Where to store the unpacked node.
 *
 */
void rexdd_fill_unpacked_from_page(
        const rexdd_nodepage_p page,
        uint_fast32_t slot,
        rexdd_unpacked_node_p node);


/****************************************************************************
 *
 *  Recycle a slot.
 *      @param  page    Page we care about
 *      @param  slot    Slot number within the page
 *
 */
void rexdd_recycle_page_slot(rexdd_nodepage_p page, uint_fast32_t slot);


/****************************************************************************
 *
 *  Dump an entire page, in human-readable format, for debugging purposes.
 *      @param  fout    Where to dump
 *      @param  page    Page struct to dump
 *
 */
void rexdd_dump_page(FILE* fout, const rexdd_nodepage_p page);


#endif