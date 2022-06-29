
#include "../unique.h"

#include <stdlib.h>

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
    for (i=0; i<10000; i++) {
        fill_node(random(), &n);
        rexdd_node_handle_t h = rexdd_nodeman_get_handle(&M, &n);
        if (h != rexdd_insert_UT(&UT, h)) {
            ++count;
        }
    }
    printf("%u duplicates detected\n", count);

    rexdd_dump_UT(stdout, &UT, true );

    rexdd_free_nodeman(&M);
    return 0;
}
