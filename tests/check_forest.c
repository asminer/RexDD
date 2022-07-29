
#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_PATTERN

void show_unpacked_node(rexdd_forest_t *F, rexdd_unpacked_node_t n)
{
    char buffer[20];

    printf("\nLevel %d node's low \n", n.level);
    rexdd_snprint_edge(buffer, 20, n.edge[0]);
    printf("\t%s\n", buffer);
    if (!rexdd_is_terminal(n.edge[0].target)) {
        printf("---------Low child-------------\n");
        rexdd_unpacked_node_t nodel;
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M,n.edge[0].target), &nodel);
        show_unpacked_node(F, nodel);
        printf("-------------------------------\n");
    }

    printf("\nLevel %d node's high \n", n.level);
    rexdd_snprint_edge(buffer, 20, n.edge[1]);
    printf("\t%s\n", buffer);
    if (!rexdd_is_terminal(n.edge[1].target)) {
        printf("---------High child-------------\n");
        rexdd_unpacked_node_t nodeh;
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M,n.edge[1].target), &nodeh);
        show_unpacked_node(F, nodeh);
        printf("-------------------------------\n");

    }

}

// void build_nodes(rexdd_forest_t *F, rexdd_unpacked_node_t *root)
// {
//     //TBD

// }



int main()
{
    /* ==========================================================================
     *      Initializing the forest, unpacked node and incoming edge for testing
     * ==========================================================================*/
    rexdd_forest_t F;
    rexdd_forest_settings_t s;

    rexdd_default_forest_settings(5, &s);
    rexdd_init_forest(&F, &s);

    rexdd_unpacked_node_t n;
    rexdd_unpacked_node_t child;
    rexdd_edge_t e;
    e.label.rule = rexdd_rule_X;
    e.label.complemented = 0;
    e.label.swapped = 0;

    printf("=================Simple pattern checking test==================\n");

    /* ==========================================================================
     *      Building the unpacked node and the number of levels
     *          TBD----Build more and different nodes and edges
     * ==========================================================================*/
    uint32_t Levels = 3;

    child.level = Levels-1;
    child.edge[0].target = rexdd_make_terminal(0x06ul);
    child.edge[0].label.rule = rexdd_rule_EH1;
    child.edge[0].label.complemented = 0;
    child.edge[0].label.swapped = 0;
    child.edge[1].target = rexdd_make_terminal(0x06ul);
    child.edge[1].label.rule = rexdd_rule_X;
    child.edge[1].label.complemented = 0;
    child.edge[1].label.swapped = 0;

    rexdd_node_handle_t h_child = rexdd_nodeman_get_handle(F.M, &child);

    n.level = Levels;
    n.edge[0].target = rexdd_make_terminal(0x06ul);
    n.edge[0].label.rule = rexdd_rule_EL0;
    n.edge[0].label.complemented = 1;
    n.edge[0].label.swapped = 0;
    n.edge[1].target = h_child;
    // n.edge[1].target = rexdd_make_terminal(0x06ul);
    n.edge[1].label.rule = rexdd_rule_X;
    n.edge[1].label.complemented = 0;
    n.edge[1].label.swapped = 0;

    rexdd_node_handle_t h = rexdd_nodeman_get_handle(F.M, &n);
    
    show_unpacked_node(&F, n);
    printf("\nnode handle is: %llu\n", h);
    
    rexdd_reduce_node(&F, &n, &e);
    printf("\nreduced edge: \n");
    char buffer[20];
    rexdd_snprint_edge(buffer, 20, e);
    printf("\t%s\n", buffer);
    if (e.target != h) {
        if (!rexdd_is_terminal(e.target)) {
            printf("---------Normalized node-------\n");
            rexdd_unpacked_node_t node;
            rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F.M,e.target), &node);
            show_unpacked_node(&F, node);
            printf("-------------------------------\n");
        }
    }

    printf("\n=================Checking eval function...===================\n");

    /* ==========================================================================
     *      Building the vars for the specific number of levels
     * ==========================================================================*/
    uint32_t num_level = Levels;

    // Build the vars for the specific number of levels
    bool vars[0x01 << num_level][num_level+1];

    int var = (0x01 << num_level) - 1;
    
    bool out;
    for (uint32_t i=0; i<(0x01 << num_level); i++) {
        vars[i][0] = 0;
        for (uint32_t j=1; j<(num_level+1); j++) {
            vars[i][j] = var & (0x01<<(j-1));
            printf("%d\t", vars[i][j]);
        }

        // Checking the eval function for the target edge
        out = rexdd_eval(&F, &e, Levels, vars[i]);
        printf("eval out: %d\n", out);
        printf("\n");
        var = var-1;
    }


    printf("\n=============================================================\n");

    rexdd_free_forest(&F);

    return 0;
}
