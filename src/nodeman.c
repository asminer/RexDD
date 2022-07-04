
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
void rexdd_init_nodeman(rexdd_nodeman_t *M, unsigned maxpages)
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
void rexdd_free_nodeman(rexdd_nodeman_t *M)
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
 *      @param  u           Unpacked node
 *
 *      @return A node handle that now stores n.
 *
 */
rexdd_node_handle_t
rexdd_nodeman_get_handle(rexdd_nodeman_t *M, const rexdd_unpacked_node_t *u)
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
        M->previous_handle = 0;
        rexdd_unpacked_to_packed(u, rexdd_get_packed_for_handle(M, h));
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


/****************************************************************************
 *
 *  Sweep a node manager.
 *  For each node in the manager, check if it is marked or not.
 *  If marked, the mark bit is cleared.
 *  If unmarked, the node is recycled.
 *      @param  M       Node manager
 *
 */
void rexdd_sweep_nodeman(rexdd_nodeman_t *M)
{
    rexdd_sanity1(M, "Null node manager");
    rexdd_sanity1(M->pages, "Empty node manager");

    /*
     * Lists of partially full pages.
     * List i will contain pages with up to 2^i free nodes.
     */
    uint_fast32_t front[32], back[32];
    unsigned i;
    for (i=0; i<32; i++) {
        front[i] = 0;
        back[i] = 0;
    }

    //
    // Go through all pages in reverse order, and build up the lists.
    //
    M->empty_pages = 0;

    uint_fast32_t p;
    for (p=M->pages_size; p; ) {
        p--;
        /* Sweep this page */
        if (M->pages[p].chunk) {
            rexdd_sweep_page(M->pages+p);
            if (0==M->pages[p].first_unalloc) {
                // This page is now empty.  Free it.
                rexdd_free_nodepage(M->pages+p);
            }
        }
        if (0==M->pages[p].chunk) {
            /*
             * Add to empty page list.
             */
            M->pages[p].next = M->empty_pages;
            M->empty_pages = p+1;
            continue;
        }

        if (0==M->pages[p].num_unused) {
            /*
             * Full page.  Doesn't go in any list.
             */
            continue;
        }

        /*
         * Partially full page.  Determine which list to add it to.
         */
        for (i=31; i; i--) {
            if (M->pages[p].num_unused & (0x01 << i)) break;
        }

        if (0==back[i]) {
            back[i] = p+1;
        }
        M->pages[p].next = front[i];
        front[i] = p+1;
    }

    //
    // Append the lists together
    //
    M->not_full_pages = front[31];
    for (i=31; i; ) {
        i--;
        if (front[i]) {
            // non-empty list; connect it to the running list.
            rexdd_sanity1(back[i], "null tail pointer");
            M->pages[back[i]-1].next = M->not_full_pages;
            M->not_full_pages = front[i];
        }
    }
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
        bool show_used, bool show_unused)
{
    rexdd_sanity1(M, "Null node manager");
    fprintf(fout, "Node manager\n");
    fprintf(fout, "    %lu pages total\n", (unsigned long)M->pages_size);
    uint_fast32_t p;
    for (p=0; p<M->pages_size; p++) {
        rexdd_dump_page(fout, M->pages+p, p, show_used, show_unused);
    }

    fprintf(fout, "    !Full list:");
    unsigned count = 0;
    for (p=M->not_full_pages; p; p=M->pages[p-1].next) {
        if (0 == count) fputs("\n\t", fout);
        fprintf(fout, "%lx (#unused %lu) -> ", (unsigned long) p-1, (unsigned long) M->pages[p-1].num_unused);
        count = (count+1) % 4;
    }
    fputs("null\n", fout);

    fprintf(fout, "    Empty list:");
    count = 0;
    for (p=M->empty_pages; p; p=M->pages[p-1].next) {
        if (0 == count) fputs("\n\t", fout);
        fprintf(fout, "%lx -> ", (unsigned long) p-1);
        count = (count+1) % 8;
    }
    fputs("null\n", fout);
}

