
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

void show_edge(rexdd_edge_t e)
{
    printf("edge's target is %llu\n", e.target);
    printf("edge rule is %d\n", e.label.rule);
    printf("edge complement bit is %d\n", e.label.complemented);
    printf("edge swap bit is %d\n", e.label.swapped);
}

void show_unpacked_node(rexdd_unpacked_node_t n)
{
    printf("node level is %d\n", n.level);
    printf("\nnode's low ");
    show_edge(n.edge[0]);
    printf("\nnode's high");
    show_edge(n.edge[1]);
    
}


int main()
{

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

        // uint64_t H = rexdd_insert_UT(F.UT, e.target);

        if (e.label.rule != rexdd_rule_N) {
            count++;
            show_unpacked_node(n);
            show_edge(e);
            break;
        }
    }

    commaprint(13, F.UT->size);
    printf(" UT size\n");

    commaprint(13, F.UT->num_entries);
    printf(" entries in UT\n");

    commaprint(13, count);
    printf(" reduced edge\n");

    printf("Checking node page handles\n");

    rexdd_unpacked_node_t new_p;
    fill_node(random(), &new_p);

    show_unpacked_node(new_p);

    rexdd_node_handle_t handle = rexdd_nodeman_get_handle(F.M, &new_p);
    rexdd_node_handle_t handle1 = rexdd_nodeman_get_handle(F.M, &new_p);

    printf("The first handle is %llu\n", handle);
    printf("The second handle is %llu\n", handle1);

    rexdd_unpacked_node_t temp;
    rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F.UT->M, handle) ,&temp);
    show_unpacked_node(temp);
    rexdd_node_handle_t hash_handle = rexdd_insert_UT(F.UT, handle);
    printf("The hashed handle is %llu\n", hash_handle);
    rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F.UT->M, hash_handle) ,&temp);
    show_unpacked_node(temp);

    printf("\nChecking nonterminals...\n");


    rexdd_free_forest(&F);




    return 0;
}