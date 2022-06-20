
#include "../nodeman.h"

#include <stdio.h>
#include <stdlib.h>

#define DUMP

const unsigned TESTS=300*1024*1024;

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

static inline uint64_t random64()
{
    uint64_t x = random();
    x <<= 31;
    return x | random();
}

static inline uint16_t randomshort()
{
    return random() & 0x00ffff;
}

void fill_random(rexdd_unpacked_node_p P)
{
    if (0==P) return;

    P->level = random();
    if (0==P->level) {
        P->level++; // level can't be zero
    }

    P->edge[0].target = random64();
    P->edge[1].target = random64();

    unsigned a = random();

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

    rexdd_nodeman_t M;
    rexdd_init_nodeman(&M, 0);

    srandom(12345678);
    rexdd_unpacked_node_t P, Q, R;

    printf("Storing random nodes (with next)...\n");

    unsigned i;
    for (i=0; i<TESTS; ++i) {
        fill_random(&P);
        copy_and_mask(&Q, &P);
    }

    bool mark, b;
    uint64_t n1, n2, n3;
    rexdd_node_handle_t h;
    for (i=0; i<TESTS; i++) {
        fill_random(&P);
        copy_and_mask(&Q, &P);

        h = rexdd_nodeman_get_handle(&M, &P);

        // random next
        n1 = random64();
        rexdd_set_next_handle(&M, h, n1);

        // random mark/unmark
        const unsigned pnum = h >> 24;
        if (pnum<16) {
            mark = (randomshort() >> (16-pnum));
        } else {
            mark = (randomshort() >> (pnum-16));
        }
        if (mark) {
            rexdd_mark_handle(&M, h);
        }

        rexdd_unpack_handle(&M, h, &R);

        if (!equal(&Q, &R)) {
            printf("Node storage test %u failed\n", i);
            print_unpacked("Original:", &P);
            print_unpacked("Masked  :", &Q);
            print_unpacked("Unpacked:", &R);
            break;
        }

        n2 = n1 & LOW49;
        n3 = rexdd_get_next_handle(&M, h);
        if (n3 != n2) {
            printf("Next test %u failed\n", i);
            printf("  Next as set: %llx\n", n1);
            printf("  Masked     : %llx\n", n2);
            printf("  Retrieved  : %llx\n", n3);
            break;
        }

        b = rexdd_is_packed_marked(rexdd_get_packed_for_handle(&M, h));
        if (b != mark) {
            printf("Mark bit mismatch on test %u\n", i);
            printf("    We think the bit is %x\n", mark);
            printf("    Packed thinks it is %x\n", b);
            break;
        }
    }
    printf("%u tests passed\n", TESTS);

    printf("Sweeping...\n");
    rexdd_sweep_nodeman(&M);
    printf("Sweeping done\n");

#ifdef DUMP
    rexdd_dump_nodeman(stdout, &M, false, false);
#endif


    rexdd_free_nodeman(&M);

    return 0;
}
