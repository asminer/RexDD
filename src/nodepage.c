#include "nodepage.h"
#include "error.h"
#include "unpacked.h"

#include <stdlib.h>
#include <assert.h>


/****************************************************************************
 *
 *  Fill a nodepage struct with zeroes.
 *
 */
void rexdd_zero_nodepage(rexdd_nodepage_p page, uint_fast32_t next)
{
    rexdd_sanity1(page, "Null page");

    page->chunk = 0;
    page->first_unalloc = 0;
    page->free_list = 0;
    page->num_unused = 0;
    page->next = next;
}


/****************************************************************************
 *
 *  Initialize a page of nodes.
 *
 */
void rexdd_init_nodepage(rexdd_nodepage_p page)
{
    rexdd_sanity1(page, "Null page");

    page->chunk = malloc(REXDD_PAGE_SIZE * sizeof(rexdd_packed_node_t));

    rexdd_check1(page->chunk, "malloc fail in nodepage");

    page->first_unalloc = 0;
    page->free_list = 0;
    page->num_unused = REXDD_PAGE_SIZE;
    page->next = 0;
}


/****************************************************************************
 *
 *  Destroy a page of nodes.
 *
 */
void rexdd_free_nodepage(rexdd_nodepage_p page)
{
    rexdd_sanity1(page, "Null page");
    free(page->chunk);
    page->chunk = 0;
}


/****************************************************************************
 *
 *  Get a free slot in a page.
 *      @param  page    Page to use.  If num_unused is zero,
 *                      rexdd_error will be called.
 *      @return The slot number used.
 *
 */
uint_fast32_t
rexdd_page_free_slot(rexdd_nodepage_p page)
{
    rexdd_sanity1(page, "Null page");
    rexdd_sanity1(page->chunk, "Slot request from unallocated page");
    rexdd_sanity1(page->num_unused, "Slot request from full page");

    --page->num_unused;

    /*
     * There are definitely free slots.
     * If the free list is not empty, pull from there.
     */
    if (page->free_list) {
        /*
         * Pull from the free list
         */
        uint_fast32_t slot = page->free_list-1;
        rexdd_sanity2(slot < REXDD_PAGE_SIZE,
                "Bad free list entry in page: %lu", slot
        );
        rexdd_sanity2(0==page->chunk[slot].fourth32,
            "Page free list entry %lu in use?", slot
        );

        page->free_list = page->chunk[slot].third32;
        return slot;
    }

    /*
     * Free list is empty, so pull from the unallocated end portion.
     */
    return page->first_unalloc++;
}


/****************************************************************************
 *
 *  Sweep a page.
 *  For each node in the page, check if it is marked or not.
 *  If marked, the mark bit is cleared.
 *  If unmarked, the node is recycled.
 *      @param  page    Page we care about
 *
 */
void rexdd_sweep_page(rexdd_nodepage_p page)
{
    rexdd_sanity1(page, "Null page");
    if (0==page->chunk) return;

    //
    // Expand the unallocated portion as much as we can
    //
    while (page->first_unalloc) {
        if (rexdd_is_packed_marked(page->chunk+page->first_unalloc-1)) break;

        page->first_unalloc--;
    }

    page->num_unused = REXDD_PAGE_SIZE - page->first_unalloc;

    //
    // Now, rebuild the free list, by scanning all nodes backwards.
    // Unmarked nodes are added to the list.
    //
    page->free_list = 0;
    unsigned i;
    for (i=page->first_unalloc; i; --i) {
        if (rexdd_is_packed_marked(page->chunk+i-1)) {
            rexdd_unmark_packed(page->chunk+i-1);
        } else {
            rexdd_recycle_packed(page->chunk+i-1, page->free_list);
            page->free_list = i;
            page->num_unused++;
        }
    }

}


/****************************************************************************
 *
 *  Dump an entire page, in human-readable format, for debugging purposes.
 *      @param  fout        Where to dump
 *      @param  page        Page struct to dump
 *      @param  pageno      Page number
 *      @param  show_used   If true, display the used nodes
 *      @param  show_unused If true, display the unused nodes
 *
 */
void rexdd_dump_page(FILE* fout, const rexdd_nodepage_p page,
        uint_fast32_t pageno, bool show_used, bool show_unused)
{
    rexdd_sanity1(page, "Null page");
    if (0==page->chunk) {
        fprintf(fout, "    Page %0x: unallocated\n", pageno);
        return;
    }
    fprintf(fout, "    Page %0x: allocated\n", pageno);
    if (page->free_list) {
        fprintf(fout, "        Free list: %x\n", (uint32_t)page->free_list-1);
    } else {
        fprintf(fout, "        Free list: null\n");
    }
    fprintf(fout, "        #unused: %u\n", (uint32_t)page->num_unused);
    uint_fast32_t i;
    rexdd_unpacked_node_t node;
    for (i=0; i<page->first_unalloc; i++) {
        if (page->chunk[i].fourth32) {
            /* In use */
            if (show_used) {
                rexdd_packed_to_unpacked(page->chunk+i, &node);
                fprintf(fout, "        Slot %x:%06x In use. level %u, ",
                    pageno, i, (unsigned) node.level);
                rexdd_fprint_edge(fout, node.edge[0]);
                fputs(", ", fout);
                rexdd_fprint_edge(fout, node.edge[1]);
                fputc('\n', fout);
            }
        } else {
            /* Freed */
            if (show_unused) {
                fprintf(fout, "        Slot %x:%06x Freed.  next ", pageno, i);
                if (page->chunk[i].third32) {
                    fprintf(fout, "%06x\n", page->chunk[i].third32-1);
                } else {
                    fputs("null\n", fout);
                }
            }
        }
    }
    if (page->first_unalloc <= 0xffffff) {
        fprintf(fout, "        Slot %x:%06x onward: unused\n", pageno, page->first_unalloc);
    }
}



