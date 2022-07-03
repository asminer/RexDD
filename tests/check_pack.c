
#include "packed.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

const unsigned TESTS=100000000;

const uint64_t LOW49= (0x01ul << 49)-1;
const uint64_t LOW50= (0x01ul << 50)-1;
const uint64_t LOW29= (0x01ul << 29)-1;
const uint64_t LOW5 = (0x01ul <<  5)-1;

bool equal(const rexdd_unpacked_node_t *P, const rexdd_unpacked_node_t *Q)
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

static inline uint16_t randombit()
{
    return random() & 0x00100;
}

void fill_random(rexdd_unpacked_node_t *P)
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

void copy_and_mask(rexdd_unpacked_node_t *dest, const rexdd_unpacked_node_t *src)
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

void print_unpacked(const char* name, const rexdd_unpacked_node_t *P)
{
    fputs(name, stdout);
    printf("\n    level: %x", P->level);
    printf("\n    edge 0:");
    rexdd_fprint_edge(stdout, P->edge[0]);
    printf("\n    edge 1:");
    rexdd_fprint_edge(stdout, P->edge[1]);
    printf("\n");
}

void check_packing(unsigned i, const rexdd_unpacked_node_t *U, const rexdd_packed_node_t *p)
{
    rexdd_unpacked_node_t V, W;
    copy_and_mask(&V, U);

    rexdd_packed_to_unpacked(p, &W);

    if (equal(&V, &W)) return;

    printf("Node packing test %u failed\n", i);
    print_unpacked("Original:", U);
    print_unpacked("Masked  :", &V);
    print_unpacked("Unpacked:", &W);

    exit(1);
}

void check_next(unsigned i, uint64_t nxt, const rexdd_packed_node_t *p)
{
    uint64_t n2 = nxt & LOW49;
    uint64_t n3 = rexdd_get_packed_next(p);
    if (n3 == n2) return;

    printf("Next test %u failed\n", i);
    printf("    Next as set: %llx\n", nxt);
    printf("    Masked     : %llx\n", n2);
    printf("    Retrieved  : %llx\n", n3);

    exit(1);
}

void check_mark(unsigned i, bool marked, const rexdd_packed_node_t *p)
{
    bool b = rexdd_is_packed_marked(p);
    if (b == marked) return;

    printf("Mark bit mismatch on test %u\n", i);
    printf("    We think the bit is %d\n", marked);
    printf("    Packed thinks it is %d\n", b);
}

int main()
{
    printf("rexdd_unpacked_node_t: %lu bytes\n", sizeof(rexdd_unpacked_node_t));
    printf("rexdd_packed_node_t: %lu bytes\n", sizeof(rexdd_packed_node_t));

    srandom(12345678);
    rexdd_unpacked_node_t U;
    rexdd_packed_node_t p;

    printf("Storing random nodes (with next)...\n");

    unsigned i;
    uint64_t next;
    bool mark;
    for (i=0; i<TESTS; ++i) {
        // Random node, next, mark
        fill_random(&U);
        next = random64();
        mark = randombit();

        rexdd_unpacked_to_packed(&U, &p);   // must come first

        if (i%2) {
            // Set next, then mark
                rexdd_set_packed_next(&p, next);
                if (mark) {
                    rexdd_mark_packed(&p);
                } else {
                    rexdd_unmark_packed(&p);
                }
        } else {
            // mark, then set next
                if (mark) {
                    rexdd_mark_packed(&p);
                } else {
                    rexdd_unmark_packed(&p);
                }
                rexdd_set_packed_next(&p, next);
        }

        //
        // Did it work?
        //

        check_packing(i, &U, &p);
        check_next(i, next, &p);
        check_mark(i, mark, &p);
    }

    printf("%u tests passed\n", TESTS);

    return 0;
}
