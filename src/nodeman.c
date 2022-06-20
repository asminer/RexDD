
#include "nodeman.h"
#include "error.h"

#include <stdlib.h>

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
void rexdd_init_nodeman(rexdd_nodeman_p M, unsigned maxpages)
{
    rexdd_sanity1(M, "Null node manager");

    if ((0==maxpages) || (maxpages > 0x01 << 25)) {
        maxpages = 0x01 << 25;
    }
    M->max_pages = maxpages;
    if (maxpages > 16) {
        M->pages_size = 16;
    } else {
        M->pages_size = maxpages;
    }

    M->pages = malloc(M->pages_size * sizeof(rexdd_nodepage_t));
    rexdd_check1(M->pages, "malloc fail in nodeman");

    M->not_full_pages = 0;
    M->empty_pages = 0;
    unsigned i;
    for (i=M->pages_size; i; i--) {
        rexdd_zero_nodepage(M->pages+i-1, M->empty_pages);
        M->empty_pages = i;
    }

    M->previous_handle = 0;
}


/****************************************************************************
 *
 *  Destroy a node manager.
 *  All memory is deallocated.
 *
 */
void rexdd_free_nodeman(rexdd_nodeman_p M)
{
    rexdd_sanity1(M, "Null node manager");
    if (0==M->pages) {
        return;
    }
    unsigned i;
    for (i=0; i<M->pages_size; i++) {
        if (M->pages[i].chunk) {
            rexdd_free_nodepage(M->pages+i);
        }
    }
    free(M->pages);

    //
    // Set everything to sane values
    //
    M->pages = 0;
    M->pages_size = 0;
    M->not_full_pages = 0;
    M->empty_pages = 0;
    M->previous_handle = 0;
}


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
rexdd_nodeman_get_handle(rexdd_nodeman_p M, const rexdd_unpacked_node_p n)
{
    rexdd_sanity1(M, "Null node manager");
    rexdd_sanity1(M->pages, "Empty node manager");

    //
    // 64 bit to avoid overflows while shifting
    //
    uint_fast64_t pnum, slot;

    //
    // Re-use the recycled handle, if we have one.
    //
    if (M->previous_handle) {
        const rexdd_node_handle_t h = M->previous_handle-1;

        slot = h & 0x00ffffff;
        pnum = h >> 24;

        rexdd_sanity2(pnum < M->max_pages, "Bad page %llu in handle", pnum);
        rexdd_unpacked_to_packed(u, M->pages[pnum].chunk+slot);
        return h;
    }

    if (0==M->not_full_pages) {
        //
        // List of partially filled pages is empty.
        // Make a new one from the list of empty pages.
        //

        if (!M->empty_pages) {
            //
            // No empty pages.  That means we've exhausted the array.
            // Time to enlarge, if we can.
            //
            if (M->pages_size >= M->max_pages) {
                // We can't enlarge.
                rexdd_error(__FILE__, __LINE__,
                    "Out of nodes in node manager; %lu pages are full", M->max_pages);
                return 0;   // Shouldn't get here
            }

            // Enlarge
            uint_fast32_t nps = M->pages_size * 2;
            if (nps > M->max_pages) {
                nps = M->max_pages;
            }
            M->pages = realloc(M->pages, nps * sizeof(rexdd_nodepage_t));
            rexdd_check1(M->pages, "realloc fail in nodeman");

            // zero out the new pages, add them to the empty list
            unsigned i;
            for (i=nps; i>M->pages_size; i--) {
                rexdd_zero_nodepage(M->pages+i-1, M->empty_pages);
                M->empty_pages = i;
            }
            M->pages_size = nps;
        }

        //
        // Remove a page from the empty list, add it to the not full list,
        // and allocate the page.
        //
        pnum = M->empty_pages-1;
        M->empty_pages = M->pages[pnum].next;
        M->pages[pnum].next = 0;
        M->not_full_pages = pnum+1;
        rexdd_init_nodepage(M->pages + pnum);
    }

    //
    // Use the list of pages with free slots.
    //
    pnum = M->not_full_pages-1;
    slot = rexdd_page_free_slot(M->pages + pnum);

    //
    // If the page is full now, remove it from the list
    //
    if (0==M->pages[pnum].num_unused) {
        M->not_full_pages = M->pages[pnum].next;
    }

    rexdd_unpacked_to_packed(u, M->pages[pnum].chunk+slot);
    return 1+ (pnum << 24 | slot);
}

