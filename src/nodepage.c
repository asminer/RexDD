#include "nodepage.h"
#include "error.h"

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

    page->chunk = malloc(REXDD_PAGE_SIZE * sizeof(rexdd_packed_node_t));
    if (0==page->chunk) {
        rexdd_error(__FILE__, __LINE__, "malloc fail in nodepage");
    }

    page->first_unalloc = 0;
    page->free_list = 0;
    page->num_unused = REXDD_PAGE_SIZE;
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
     * If the free list is not empty, pull from there.
     */
    uint_fast32_t slot;
    if (page->free_list) {
        /*
         * Pull from the free list
         */
        slot = page->free_list-1;
        if (slot >= REXDD_PAGE_SIZE) {
            rexdd_error(__FILE__, __LINE__, "Bad free list entry in page: %lu",
                slot);
        }
        if (page->chunk[slot].fourth32) {
            rexdd_error(__FILE__, __LINE__, "Page free list entry %lu in use?",
                slot);
        }
        page->free_list = page->chunk[slot].third32;
    } else {
        /*
         * Free list is empty, so pull from the unallocated end portion.
         */
        slot = page->first_unalloc++;
    }
    --page->num_unused;


    /*
     * We have a free slot; now fill it with the given node.
     */
    rexdd_unpacked_to_packed(node, page->chunk+slot);

    return slot;
}

//
// Helper: show an edge
//
static inline void rexdd_show_edge(FILE* fout, rexdd_edge_t e)
{
    fprintf(fout, "<%s,%c,%c,%llu>",
            rexdd_rule_name[e.label.rule],
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            e.target
    );
}

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
        bool show_used, bool show_unused)
{
    if (0==page) {
        fprintf(fout, "    Null page\n");
        return;
    }
    if (0==page->chunk) {
        fprintf(fout, "    Unallocated page\n");
        return;
    }
    if (page->free_list) {
        fprintf(fout, "    Free list: %llu\n", (uint64_t)page->free_list-1);
    } else {
        fprintf(fout, "    Free list: null\n");
    }
    fprintf(fout, "    #unused: %llu", (uint64_t)page->num_unused);
    uint_fast32_t i;
    rexdd_unpacked_node_t node;
    for (i=0; i<page->first_unalloc; i++) {
        if (page->chunk[i].fourth32) {
            /* In use */
            if (show_used) {
                rexdd_fill_unpacked_from_page(page, i, &node);
                fprintf(fout, "    Slot %-8u: lvl %u, ",
                    i, (unsigned) node.level);
                rexdd_show_edge(fout, node.edge[0]);
                fputs(", ", fout);
                rexdd_show_edge(fout, node.edge[1]);
                fputc('\n', fout);
            }
        } else {
            /* Freed */
            if (show_unused) {
                fprintf(fout, "    Slot %-8u: free; next %u\n", i,
                        page->chunk[i].third32);
            }
        }
    }
    fprintf(fout, "    Slot %-8u onward: unused\n", page->first_unalloc);
}



