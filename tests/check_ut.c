
#include "unique.h"

#include <stdlib.h>

#define HSIZE 1000

const unsigned insertions = 10000;
// const unsigned insertions = 1000000;
const unsigned utmax = 0x01 << 21;

/*
 * Note - there's about 21 bits used here,
 * i.e., at most about 2 million possible nodes.
 * So if we keep adding to the UT, eventually we should
 * max out at 2^21 nodes.
 *
 */
void fill_node(unsigned seed, rexdd_unpacked_node_t *node)
{
    node->edge[0].target = seed & 0xf;
    seed >>= 4;
    node->edge[1].target = seed & 0xf;
    seed >>= 4;

    node->edge[0].label.rule = seed & 0x7;
    seed >>= 3;
    node->edge[1].label.rule = seed & 0x7;
    seed >>= 3;

    node->edge[0].label.complemented = 0;
    node->edge[0].label.swapped = seed & 0x01;
    seed >>= 1;
    node->edge[1].label.complemented = seed & 0x01;
    seed >>= 1;
    node->edge[1].label.swapped = seed & 0x01;
    seed >>= 1;
    node->level = 1+ (seed & 0xf);
}

int main()
{
    srandom(12345678);

    rexdd_nodeman_t M;
    rexdd_unique_table_t UT;
    rexdd_unpacked_node_t n;

    rexdd_init_nodeman(&M, 2);
    rexdd_create_UT(&UT, &M);

    unsigned i, count = 0;
    for (i=0; i<insertions; i++) {
        fill_node(random(), &n);
        rexdd_node_handle_t h = rexdd_nodeman_get_handle(&M, &n);
        rexdd_node_handle_t g = rexdd_insert_UT(&UT, h);
        if (g != h) {
            ++count;
        }
        if (UT.num_entries + count != i+1) {
            printf("  failed insertion on item %u?\n", i);
            printf("      orig handle: %llu\n", h);
            printf("        UT handle: %llu\n", g);
            return 1;
        }
    }
    printf("%u duplicates detected\n", count);

    printf("%llu entries in UT\n", UT.num_entries);

    if (UT.num_entries > utmax) {
        printf("   hmm, expected %u entries at most?\n", utmax);
        return 1;
    }

    unsigned total = count + UT.num_entries;

    printf("%u total\n", total);

    printf("Histogram of (final) chain lengths:\n");

    uint_fast64_t histogram[HSIZE];

    rexdd_histogram_UT(&UT, histogram, HSIZE);

    uint_fast64_t harea = 0;
    for (i=0; i<HSIZE; i++) {
        if (0==histogram[i]) continue;
        printf("%5u: %llu\n", i, histogram[i]);
        harea += i*histogram[i];
    }
    printf("Unaccounted for chain items: %llu\n", UT.num_entries - harea);

    rexdd_free_nodeman(&M);
    return 0;
}
