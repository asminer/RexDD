
#include "nodepage.h"

#include <stdlib.h>

// #define DUMP_NODES

/*
 * Checks nodepage stuff
 */

unsigned marklist[REXDD_PAGE_SIZE];

double uniform()
{
    // random returns between 0 and 2^31 - 1
    double x;
    do {
        x = rand();
    } while (0.0 == x);
    return x / 2147483648.0;
}

unsigned equilikely(unsigned a, unsigned b)
{
    return a + (unsigned)( (b-a+1) * uniform() );
}

void randomize_marklist(const rexdd_nodepage_t *P)
{
    unsigned i, j, mlen, t;
    for (i=0; i<REXDD_PAGE_SIZE; i++) {
        marklist[i] = 0;
    }
    mlen = 0;
    for (i=0; i<P->first_unalloc; i++) {
        if (rexdd_is_packed_in_use(P->chunk + i)) {
            marklist[mlen++] = i+1;
        }
    }
    for (i=0; i<mlen; i++) {
        j = equilikely(i, mlen-1);
        if (i != j) {
            t = marklist[i];
            marklist[i] = marklist[j];
            marklist[j] = t;
        }
    }
}

unsigned max_marked_plus1(unsigned slots)
{
    unsigned i, m=0;
    for (i=0; i<slots; i++) {
        if (marklist[i] > m)
            m = marklist[i];
    }
    return m;
}


void alloc_mark_sweep(rexdd_nodepage_t *page, unsigned num_a, unsigned num_m)
{
    // Set up a bogus unpacked node
    // (needed so mark bit will be correct)
    rexdd_unpacked_node_t u;
    u.level=2;
    rexdd_set_edge(u.edge+0, 1, false, false, num_a);
    rexdd_set_edge(u.edge+1, 2, false, false, num_m);

    //
    // Allocate
    //
    printf("  Allocating %u nodes\n", num_a);
    unsigned lasth = 0;
    for (; num_a; num_a--) {
        lasth = rexdd_page_free_slot(page);
        rexdd_unpacked_to_packed(&u, page->chunk + lasth);
    }

#ifdef DUMP_NODES
    rexdd_dump_page(stdout, page, 0, true, true);
#endif

    //
    // Show number of used
    //
    printf("%u nodes used, %u nodes available\n",
            REXDD_PAGE_SIZE - page->num_unused, page->num_unused);

    //
    // Randomly mark
    //
    printf("  Marking %u nodes\n", num_m);
    randomize_marklist(page);
    unsigned i;
    for (i=0; i<num_m; i++) {
        if (0==marklist[i]) continue;
        rexdd_mark_packed( page->chunk + marklist[i]-1 );
#ifdef DUMP_NODES
        printf("Marked %u\n", marklist[i]-1);
#endif
    }
    //
    // Sweep
    //
    rexdd_sweep_page(page);

#ifdef DUMP_NODES
    rexdd_dump_page(stdout, page, 0, true, true);
#endif

}

void check_equal(const char* what, unsigned a, unsigned b)
{
    if (a==b) return;
    printf("%s mismatch: expected %u, was %u\n", what, a, b);
    exit(1);
}

int main()
{
    printf("Node page test\n\n");

    const unsigned allox[] = {
        16, 32, 1, 64, 128, 256, 512, 1024, 2048, 4092, 8192, 16384,
        32768, 65536, 131072, 262144, 524288, 1048576, 2097152,
        4194394, 8388608, 16770000,       1,       1,       1,
              1,     1,     1,     1,    1,    1,   1,  1,  1, 1, 1, 1, 0
    };
    const unsigned marks[] = {
         8, 12, 12, 1,  80, 300,  42, 1000, 3000, 5000,   10,     1,
        32768, 98304, 100000, 200000, 400000,  100000,   25000,
          12000,    7216, 16777215, 8388607, 4194393, 2097151,
        1048575, 262143, 65535, 16383, 4091, 1023, 255, 63, 15, 3, 1, 1, 0
    };

    srand(3263827);   // if you know, you know

    rexdd_nodepage_t P;

    rexdd_init_nodepage(&P);

    unsigned i;
    for (i=0; allox[i]; i++) {
        alloc_mark_sweep(&P, allox[i], marks[i]);
        check_equal("used nodes", marks[i], REXDD_PAGE_SIZE - P.num_unused);
        check_equal("first unalloc", max_marked_plus1(marks[i]), P.first_unalloc);
    }

    rexdd_free_nodepage(&P);

    printf("Done\n");

    return 0;
}
