
#include "nodepage.h"

#include <stdlib.h>

// #define DUMP_NODES

/*
 * Checks nodepage stuff
 */

unsigned marked[REXDD_PAGE_SIZE];

double uniform()
{
    // random returns between 0 and 2^31 - 1
    double x;
    do {
        x = random();
    } while (0.0 == x);
    return x / 2147483648.0;
}

unsigned equilikely(unsigned a, unsigned b)
{
    return a + (unsigned)( (b-a+1) * uniform() );
}

void randomize_marked(unsigned last)
{
    unsigned i, j, t;
    for (i=0; i < REXDD_PAGE_SIZE; i++) {
        marked[i] = i;
    }
    for (i=0; i <= last; i++) {
        j = equilikely(i, last);
        if (j > last) {
            printf("equilikely bounds error\n");
            exit(1);
        }
        if (i != j) {
            t = marked[i];
            marked[i] = marked[j];
            marked[j] = t;
        }
    }
}


void alloc_mark_sweep(rexdd_nodepage_t *page, unsigned num_a, unsigned num_m)
{
    // Set up a bogus unpacked node
    // (needed so mark bit will be correct)
    rexdd_unpacked_node_t u;
    u.level=2;
    u.edge[0].target = num_a;
    u.edge[0].label.rule = 1;
    u.edge[0].label.complemented = false;
    u.edge[0].label.swapped = false;

    u.edge[1].target = num_m;
    u.edge[1].label.rule = 2;
    u.edge[1].label.complemented = false;
    u.edge[1].label.swapped = false;

    //
    // Allocate
    //
    printf("Allocating %u nodes\n", num_a);
    unsigned lasth = 0;
    for (; num_a; num_a--) {
        lasth = rexdd_page_free_slot(page);
        rexdd_unpacked_to_packed(&u, page->chunk + lasth);
    }

#ifdef DUMP_NODES
    rexdd_dump_page(stdout, page, 0, true, true);
#endif

    //
    // Randomly mark
    //
    printf("Marking %u nodes\n", num_m);
    randomize_marked(lasth);
    unsigned i;
    for (i=0; i<num_m; i++) {
        rexdd_mark_packed( page->chunk + marked[i] );
#ifdef DUMP_NODES
        printf("Marked %u\n", marked[i]);
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
    srandom(3263827);   // if you know, you know

    rexdd_nodepage_t P;

    rexdd_init_nodepage(&P);

    alloc_mark_sweep(&P, 16, 8);
    check_equal("used nodes", 8, REXDD_PAGE_SIZE - P.num_unused);

    alloc_mark_sweep(&P, 32, 12);
    check_equal("used nodes", 12, REXDD_PAGE_SIZE - P.num_unused);

    // TBD - more tests here

    rexdd_free_nodepage(&P);

    printf("Done\n");

    return 0;
}
