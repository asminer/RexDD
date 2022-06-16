#include "nodepage.h"

#include <stdlib.h>
#include <assert.h>

/****************************************************************************
 *
 *  Initialize a page of nodes.
 *
 */
void rexdd_init_nodepage(rexdd_nodepage_p page)
{
    if (0==page) {
        rexdd_error(__FILE__, __LINE__, "Null page");
    }

    page->chunk = malloc(REXDD_PAGESIZE * sizeof(rexdd_packed_node_t));
    if (0==page->chunk) {
        rexdd_error(__FILE__, __LINE__, "malloc fail in nodepage");
    }

    page->first_unalloc = 0;
    page->free_list = 0;
    page->num_unused = REXDD_PAGESIZE;
    page->next = 0;   // Unsure about this one
}


/****************************************************************************
 *
 *  Destroy a page of nodes.
 *
 */
void rexdd_free_nodepage(rexdd_nodepage_p page)
{
    if (0==page) {
        rexdd_error(__FILE__, __LINE__, "Null page");
    }
    if (0==page->chunk) {
        rexdd_error(__FILE__, __LINE__, "Freed empty node page");
    }
    free(page->chunk);
    page->chunk = 0;
}


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
        const rexdd_unpacked_node_p node)
{
    if (0==page) {
        rexdd_error(__FILE__, __LINE__, "Null page");
    }
    if (0==page->num_unused) {
        rexdd_error(__FILE__, __LINE__, "Slot request from full page");
    }

    /*
     * There are definitely free slots.
     * Take from the free list first.
     */
    uint_fast32_t slot = ~0;
    uint_fast32_t front;
    while (page->free_list) {
        /*
         * Pull from the front of the free list.  However, if that
         * slot is adjacent to the unused portion, add it to the
         * unused portion and go to the next element in the free list.
         */
        front = page->free_list;
        slot = front-1;
        if (front >= REXDD_PAGE_SIZE) {
            rexdd_error(__FILE__, __LINE__, "Bad free list entry in page: %lu",
                front);
        }
        if (page->chunk[slot].fourth32) {
            rexdd_error(__FILE__, __LINE__, "Page free list entry %lu in use?",
                front);
        }
        page->free_list = page->chunk[slot].third32;

        if (front < page->first_unalloc) {
            break;
        }
        --page->first_unalloc;
    }

    if (slot >= page->first_unalloc) {
        /*
         * We couldn't re-use from the free list, so pull
         * from the unallocated end portion.
         */
        slot = page->first_unalloc++;
    }
    --page->num_unused;

    assert(slot != ~0);

    /*
     * We have a free slot; now fill it with the given node.
     */
    rexdd_unpacked_to_packed(node, page->chunk+slot);

    return slot;
}


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

