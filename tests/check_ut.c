
#include "unique.h"

#include <stdlib.h>
#include <string.h>

#define SHOW_HISTOGRAM

#define HSIZE 1000

const unsigned insertions = 10000000;
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

void commaprint(int width, uint_fast64_t a)
{
    char buffer[22];
    snprintf(buffer, 22, "%lu", (unsigned long) a);
    buffer[21] = 0;
    unsigned digs = strlen(buffer);

    int i;
    for (i=digs + (digs-1)/3; i < width; i++) {
        fputc(' ', stdout);
    }
    unsigned comma = digs % 3;
    if (0==comma) comma += 3;
    for (i=0; buffer[i]; i++) {
        if (0==comma) {
            fputc(',', stdout);
            comma = 2;
        } else {
            --comma;
        }
        fputc(buffer[i], stdout);
    }

    for (i=digs + (digs-1)/3; i < -width; i++) {
        fputc(' ', stdout);
    }
}


int main()
{
    printf("Unique table test\n\n");

    srand(12345678);

    rexdd_nodeman_t M;
    rexdd_unique_table_t UT;
    rexdd_unpacked_node_t n;

    rexdd_init_nodeman(&M, 2);
    rexdd_init_UT(&UT, &M);

    unsigned i, count = 0;
    for (i=0; i<insertions; i++) {
        fill_node(rand(), &n);
        rexdd_node_handle_t h = rexdd_nodeman_get_handle(&M, &n);
        rexdd_node_handle_t g = rexdd_insert_UT(&UT, h);
        if (g != h) {
            ++count;
        }
        if (UT.num_entries + count != i+1) {
            printf("  failed insertion on item %u?\n", i);
            printf("      orig handle: %lu\n", (unsigned long) h);
            printf("        UT handle: %lu\n", (unsigned long) g);
            return 1;
        }
    }
    commaprint(13, UT.size);
    printf(" UT size\n");

    commaprint(13, 0x01 << 21);
    printf(" max possible entries\n\n");

    commaprint(13, UT.num_entries);
    printf(" entries in UT\n");

    commaprint(13, count);
    printf(" duplicates detected\n");

    if (UT.num_entries > utmax) {
        printf("   hmm, expected %u entries at most?\n", utmax);
        return 1;
    }

    unsigned total = count + UT.num_entries;

    commaprint(13, total);
    printf(" total\n");

#ifdef SHOW_HISTOGRAM
    printf("Histogram of (final) chain lengths:\n");

    uint_fast64_t histogram[HSIZE];

    rexdd_histogram_UT(&UT, histogram, HSIZE);

    for (i=0; i<HSIZE; i++) {
        if (0==histogram[i]) continue;
        printf("%5u: ", i);
        commaprint(13, histogram[i]);
        printf("\n");
    }
#endif

    rexdd_free_UT(&UT);
    rexdd_free_nodeman(&M);
    return 0;
}
