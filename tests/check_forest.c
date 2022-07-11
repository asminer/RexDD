
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

// check if the target type is true
bool all_exist(bool var[], int n, rexdd_rule_t type)
{
    rexdd_sanity1((2 <= type && type <= 13), "edge rule wrong");

    bool all_lows = 0, exist_lows = 1;      // ture: 0; false: 1
    
    for (int i=0; i<n; i++) {
        all_lows = all_lows | var[i];       // |: 0, 1; &: 0, 0/1
        exist_lows = exist_lows & var[i];   // |: 1/0, 1; &: 0, 1
    }
    bool result;
    // AL:0, AH:1, EL:2, EH:3
    switch (type)
    {
    case rexdd_rule_ALN:
        result = !all_lows;
        break;
    case rexdd_rule_ALZ:
        result = all_lows;
        break;
    case rexdd_rule_AHN:
        result = exist_lows;
        break;
    case rexdd_rule_AHZ:
        result = !exist_lows;
        break;
    case rexdd_rule_ELN:
        result = !exist_lows;
        break;
    case rexdd_rule_ELZ:
        result = exist_lows;
        break;
    case rexdd_rule_EHN:
        result = all_lows;
        break;
    case rexdd_rule_EHZ:
        result = !all_lows;
        break;
    case rexdd_rule_LN:
        result = (!all_lows) | (!exist_lows);
        break;
    case rexdd_rule_LZ:
        result = (all_lows) | (exist_lows);
        break;
    case rexdd_rule_HN:
        result = (exist_lows) | (all_lows);
        break;
    case rexdd_rule_HZ:
        result = (!exist_lows) | (!all_lows);
        break;
    default:
        result = 0;
        break;
    }

    return result;
}

// void build_nodes(rexdd_forest_t *F, rexdd_unpacked_node_t *root)
// {
//     //TBD

// }

bool eval(rexdd_edge_t *e, bool var[], uint32_t edge_level) // x1, x2,..., xn;
{
    // rexdd_sanity1(edge_level == sizeof(var)/sizeof(var[0]) + 1, "edge level wrong\n");
    bool out = 0;
    if (!rexdd_is_terminal(e->target)) {
        if (e->label.rule == rexdd_rule_N || e->label.rule == rexdd_rule_X) {
            return e->label.complemented;
        } else {
            if (e->label.complemented) {
                out = !all_exist(var, edge_level-1, e->label.rule);
            } else {
                out = all_exist(var, edge_level-1, e->label.rule);
            }
        }
    } else {
        // TBD

    }
    return out;
    // int comlemented_count = 0;
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

    unsigned count = 0;
    for (unsigned i=0; i < num_nodes; i++) {
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

    printf("Checking one level nodes...\n");

    int num_nodes1 = 4 + 1;  // 1 rule (N) * 2 * 2 (complement) + 1
    rexdd_unpacked_node_t one_level_nodes[num_nodes1];
    rexdd_node_handle_t handles[num_nodes1];

    // initialize level one nodes
    printf("Initializing level one nodes...\n");
    for (int i=0; i<num_nodes1; i++) {
        fill_node(random(), &one_level_nodes[i], 1);
        one_level_nodes[i].level = 1;
        one_level_nodes[i].edge[0].label.swapped = 0;
        one_level_nodes[i].edge[1].label.swapped = 0;
        handles[num_nodes1-1] = rand()%5;
    }

    // build terminal node
    handles[num_nodes1-1] = rexdd_make_terminal(handles[num_nodes1-1]);
    printf("Building terminal node: %d\n", rexdd_is_terminal(handles[num_nodes1-1]));
    one_level_nodes[num_nodes1-1].level = 0;

    // build level one nodes
    printf("Building level one node edges...\n");
    for (int i=0; i<num_nodes1-1; i++) {
        one_level_nodes[i].edge[0].target = handles[num_nodes1-1];
        one_level_nodes[i].edge[1].target = handles[num_nodes1-1];
        one_level_nodes[i].edge[0].label.rule = rexdd_rule_N;
        one_level_nodes[i].edge[1].label.rule = rexdd_rule_N;
        if (i/2==0) {
            one_level_nodes[i].edge[0].label.complemented = 0;
        } else {
            one_level_nodes[i].edge[0].label.complemented = 1;
        }
        if (i%2==0) {
            one_level_nodes[i].edge[1].label.complemented = 0;
        } else {
            one_level_nodes[i].edge[1].label.complemented = 1;
        }
        handles[i] = rexdd_nodeman_get_handle(F.M, &one_level_nodes[i]);
    }

    // simple test
    printf("test...\n");
    rexdd_edge_t one_var, reduced;
    one_var.target = handles[num_nodes1-1];
    one_var.label.complemented = 0;
    one_var.label.swapped = 0;
    one_var.label.rule = rexdd_rule_N;
    reduced = one_var;

    bool true_varsZ[4] = {0,0,1,1};
    // bool true_varsN[4] = {0,1,0,1};

    bool var_test[2] = {0, 0};
    for (int i=0; i<num_nodes1-1; i++) {
        // reduce edge
        printf("showing edge %d...\n", i);
        show_unpacked_node(one_level_nodes[i]);
        printf("showing handle of node %d: %llu\n", i, handles[i]);

        rexdd_check_pattern(&F, &one_level_nodes[i], &reduced);

        if (eval(&reduced, var_test[1], 2) == true_varsZ[i]) {
            continue;
        } else {
            printf("eval wrong!\n");
            break;
        }
        // rexdd_reduce_edge(&F, 1, one_var.label, one_level_nodes[i], &reduced);
        // show_edge(reduced);
        // reduced = one_var;
    }



    rexdd_free_forest(&F);




    return 0;
}
