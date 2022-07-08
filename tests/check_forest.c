
#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_PATTERN

const unsigned num_nodes = 1000000;

void fill_node(unsigned seed, rexdd_unpacked_node_t *node, bool is_term)
{
    // TBD not random
    if (!is_term){
        node->edge[0].target = seed & 0xf;
        seed >>= 4;
        node->edge[1].target = seed & 0xf;
        seed >>= 4;
    } else {
        int a = rand()%3;
        if (a == 0) {
            node->edge[0].target = (seed & 0xf) | (0x01ul << 49);
            seed >>= 4;
            node->edge[1].target = (seed & 0xf) | (0x01ul << 49);
            seed >>= 4;
        } else if (a == 1) {
            node->edge[0].target = (seed & 0xf) | (0x01ul << 49);
            seed >>= 4;
            node->edge[1].target = seed & 0xf;
            seed >>= 4;
        } else {
            node->edge[0].target = seed & 0xf;
            seed >>= 4;
            node->edge[1].target = (seed & 0xf) | (0x01ul << 49);
            seed >>= 4;
        }

    }

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

void show_edge(rexdd_edge_t e)
{
    if (e.target >= (0x01ul<<49)) {
        commaprint(13, e.target - (0x01ul<<49));
        printf(" edge's target (nonterminal)\n");
    }else {
        commaprint(13, e.target);
        printf(" edge's target (terminal)\n");
    }
    commaprint(13,e.label.rule);
    printf(" edge rule\n");
    commaprint(13, e.label.complemented);
    printf(" edge complement bit\n");
    commaprint(13, e.label.swapped);
    printf(" edge swap bit\n");
}

void show_unpacked_node(rexdd_unpacked_node_t n)
{
    printf("node level is %d\n", n.level);
    printf("\nnode's low \n");
    show_edge(n.edge[0]);
    printf("\nnode's high \n");
    show_edge(n.edge[1]);
    
}


int main()
{
    srandom(123456789);
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
        fill_node(random(), &n, 1);
        rexdd_check_pattern(&F, &n, &e);

        // uint64_t H = rexdd_insert_UT(F.UT, e.target);

        if (e.label.rule != rexdd_rule_N) {
            count++;
            show_unpacked_node(n);
            printf("\nreduced edge: \n");
            show_edge(e);
            printf("\n-------------------normalizing-------------------\n");
            rexdd_normalize_edge(&n, &e);
            show_unpacked_node(n);
            printf("\nnormalized edge: \n");
            show_edge(e);
            break;
        }
    }
    printf("\n========================================\n");

    // 1. check the function values
    // 2. check if the return edge is correct
    

    rexdd_free_forest(&F);




    return 0;
}