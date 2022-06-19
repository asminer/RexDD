#ifndef NODEPAGE_H
#define NODEPAGE_H

#include "unpacked.h"
#include "packed.h"

#include <stdio.h>

#define REXDD_PAGE_SIZE (16*1024*1024)

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

    /*  pointer (index) for various lists of pages.
     *  Used by node manager structs.  */
    uint_fast32_t next;

} rexdd_nodepage_t;

typedef rexdd_nodepage_t* rexdd_nodepage_p;


/****************************************************************************
 *
 *  Fill a nodepage struct with zeroes.
 *      @param  page    Page to set.
 *      @param  next    Next pointer.
 *
 */
void rexdd_zero_nodepage(rexdd_nodepage_p page, uint_fast32_t next);

/****************************************************************************
 *
 *  Initialize a page of nodes.
 *  The chunk is allocated, and every slot is set to "unused".
 *
 */
void rexdd_init_nodepage(rexdd_nodepage_p page);


/****************************************************************************
 *
 *  Destroy a page of nodes.
 *  The chunk is deallocated.
 *
 */
void rexdd_free_nodepage(rexdd_nodepage_p page);


/****************************************************************************
 *
 *  Get a free slot in a page.
 *      @param  page    Page to use.  If num_unused is zero,
 *                      rexdd_error will be called.
 *      @return The slot number used.
 *
 */
uint_fast32_t
rexdd_page_free_slot(rexdd_nodepage_p page);


/****************************************************************************
 *
 *  Sweep a page.
 *  For each node in the page, check if it is marked or not.
 *  If marked, the mark bit is cleared.
 *  If unmarked, the node is recycled.
 *      @param  page    Page we care about
 *
 */
void rexdd_sweep_page(rexdd_nodepage_p page);



/****************************************************************************
 *
 *  Dump an entire page, in human-readable format, for debugging purposes.
 *      @param  fout        Where to dump
 *      @param  page        Page struct to dump
 *      @param  show_used   If true, display the used nodes
 *      @param  show_unused If true, display the unused nodes
 *
 */
void rexdd_dump_page(FILE* fout, const rexdd_nodepage_p page,
        bool show_used, bool show_unused);


#endif
