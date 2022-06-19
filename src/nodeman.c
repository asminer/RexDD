
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
}


