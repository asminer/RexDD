
#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned num_nodes = 1000000;

void fill_node(unsigned seed, rexdd_unpacked_node_t *node)
{
    // TBD not random
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
    srandom(123456);

    rexdd_forest_t F;
    rexdd_forest_settings_t s;
    
    rexdd_default_forest_settings(5, &s);
    rexdd_init_forest(&F, &s);

    rexdd_unpacked_node_t n;
    rexdd_edge_t e;
    e.label.rule = rexdd_rule_N;
    e.label.complemented = 0;
    e.label.swapped = 0;

    unsigned i, count = 0;
    for (i=0; i < num_nodes; i++) {
        fill_node(random(), &n);
        rexdd_check_pattern(&F, &n, &e);
        if (e.label.rule != rexdd_rule_N) count++;

    }

    commaprint(13, F.UT->size);
    printf(" UT size\n");

    commaprint(13, F.UT->num_entries);
    printf(" entries in UT\n");

    commaprint(13, count);
    printf(" reduced edge\n");




    return 0;
}