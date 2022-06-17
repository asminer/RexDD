
#include "../nodepage.h"

#include <stdio.h>
#include <stdlib.h>

const unsigned TESTS=10;

const uint64_t LOW49= (0x01ul << 49)-1;
const uint64_t LOW50= (0x01ul << 50)-1;
const uint64_t LOW29= (0x01ul << 29)-1;
const uint64_t LOW5 = (0x01ul <<  5)-1;

bool equal(const rexdd_unpacked_node_p P, const rexdd_unpacked_node_p Q)
{
    if (0==P) return false;
    if (0==Q) return false;

    if (P->level != Q->level) return false;

    if (P->edge[0].target != Q->edge[0].target)
            return false;
    if (P->edge[0].label.rule != Q->edge[0].label.rule)
            return false;
    if (P->edge[0].label.complemented != Q->edge[0].label.complemented)
            return false;
    if (P->edge[0].label.swapped != Q->edge[0].label.swapped)
            return false;

    if (P->edge[1].target != Q->edge[1].target)
            return false;
    if (P->edge[1].label.rule != Q->edge[1].label.rule)
            return false;
    if (P->edge[1].label.complemented != Q->edge[1].label.complemented)
            return false;
    if (P->edge[1].label.swapped != Q->edge[1].label.swapped)
            return false;

    return true;
}

void fill_random(rexdd_unpacked_node_p P)
{
    if (0==P) return;

    unsigned a = random();  // 31 random bits
    if (0==a) ++a;
    P->level = a;

    a = random();
    unsigned b = random();

    uint64_t x = a;
    P->edge[0].target = (x << 31) | b;

    x = random();
    b = random();

    P->edge[1].target = (x << 31) | b;

    a = random();

    P->edge[0].label.rule = a & LOW5;
    a >>= 5;
    P->edge[1].label.rule = a & LOW5;
    a >>= 5;
    P->edge[0].label.complemented = 0;
    P->edge[1].label.complemented = a & 0x01;
    a >>= 1;
    P->edge[0].label.swapped = a & 0x01;
    a >>= 1;
    P->edge[1].label.swapped = a & 0x01;
}

void copy_and_mask(rexdd_unpacked_node_p dest, const rexdd_unpacked_node_p src)
{
    dest->level = src->level & LOW29;
    dest->edge[0].target = src->edge[0].target & LOW50;
    dest->edge[1].target = src->edge[1].target & LOW50;

    dest->edge[0].label.rule = src->edge[0].label.rule & LOW5;
    dest->edge[1].label.rule = src->edge[1].label.rule & LOW5;

    dest->edge[0].label.complemented = src->edge[0].label.complemented;
    dest->edge[1].label.complemented = src->edge[1].label.complemented;

    dest->edge[0].label.swapped = src->edge[0].label.swapped;
    dest->edge[1].label.swapped = src->edge[1].label.swapped;
}

void print_unpacked(const char* name, const rexdd_unpacked_node_p P)
{
    fputs(name, stdout);
    printf("\n  level: %x", P->level);
    printf("\n  edge 0: <%x %x %x %llx>",
        P->edge[0].label.rule,
        P->edge[0].label.complemented,
        P->edge[0].label.swapped,
        P->edge[0].target
    );
    printf("\n  edge 1: <%x %x %x %llx>\n\n",
        P->edge[1].label.rule,
        P->edge[1].label.complemented,
        P->edge[1].label.swapped,
        P->edge[1].target
    );
}

int main()
{
    printf("rexdd_unpacked_node_t: %lu bytes\n", sizeof(rexdd_unpacked_node_t));
    printf("rexdd_packed_node_t: %lu bytes\n", sizeof(rexdd_packed_node_t));

    rexdd_nodepage_t page;
    rexdd_init_nodepage(&page);

    srandom(12345678);
    rexdd_unpacked_node_t P, Q, R;

    uint_fast32_t h;
    unsigned i;
    for (i=0; i<TESTS; i++) {
        fill_random(&P);
        copy_and_mask(&Q, &P);

        h = rexdd_fill_free_page_slot(&page, &Q);

        rexdd_fill_unpacked_from_page(&page, h, &R);

        if (!equal(&Q, &R)) {
            printf("Test %u failed\n", i);
            print_unpacked("Original:", &P);
            print_unpacked("Masked  :", &Q);
            print_unpacked("Unpacked:", &R);
            break;
        }

        rexdd_recycle_page_slot(&page, h);
    }

    rexdd_free_nodepage(&page);

    return 0;
}
