
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

void fprint_rexdd (FILE *f, rexdd_forest_t *F, rexdd_edge_t e)
{   
    char label_bufferL[10];
    char label_bufferH[10];

    rexdd_unpacked_node_t node;
    rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M,e.target), &node);
    fprintf(f,"\t{rank=same v%d %llu [label = \"N%llu_%llu\", shape = circle]}\n",
                rexdd_unpack_level(rexdd_get_packed_for_handle(F->M,e.target)),
                e.target,
                e.target / REXDD_PAGE_SIZE,
                e.target % REXDD_PAGE_SIZE);
    
    snprintf(label_bufferL, 10, "<%s,%c,%c>",
            rexdd_rule_name[node.edge[0].label.rule],
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_'
        );
    snprintf(label_bufferH, 10, "<%s,%c,%c>",
            rexdd_rule_name[node.edge[1].label.rule],
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_'
        );
    if (rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target)) {
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = dashed label = \"%s\"]\n",
                    e.target,
                    label_bufferL);
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = solid label = \"%s\"]\n",
                    e.target,
                    label_bufferH);
    } else if (!rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target)) {
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = dashed label = \"%s\"]\n",
                    e.target,
                    label_bufferL);
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = solid label = \"%s\"]\n",
                    e.target,
                    node.edge[1].target,
                    label_bufferH);
        fprint_rexdd(f, F, node.edge[1]);
    } else if (rexdd_is_terminal(node.edge[1].target) && !rexdd_is_terminal(node.edge[0].target)) {
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = dashed label = \"%s\"]\n",
                    e.target,
                    node.edge[0].target,
                    label_bufferL);
        fprint_rexdd(f, F, node.edge[0]);
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = solid label = \"%s\"]\n",
                    e.target,
                    label_bufferH);
    } else {
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = dashed label = \"%s\"]\n",
                    e.target,
                    node.edge[0].target,
                    label_bufferL);
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = solid label = \"%s\"]\n",
                    e.target,
                    node.edge[1].target,
                    label_bufferH);
        if (node.edge[0].target != node.edge[1].target) {
            fprint_rexdd(f, F, node.edge[0]);
            fprint_rexdd(f, F, node.edge[1]);
        } else {
            fprint_rexdd(f, F, node.edge[1]);
        }
    }


    
    
}

void build_gv(FILE *f, rexdd_forest_t *F, rexdd_edge_t e)
{
    fprintf(f, "digraph g\n{\n");
    fprintf(f, "\trankdir=TB\n");

    fprintf(f, "\n\tnode [shape = none]\n");
    fprintf(f, "\tv0 [label=\"\"]\n");
    for (uint32_t i=1; i<=rexdd_unpack_level(rexdd_get_packed_for_handle(F->M,e.target)); i++) {
        fprintf(f, "\tv%d [label=\"x%d\"]\n", i, i);
        fprintf(f, "\tv%d -> v%d [style=invis]\n", i, i-1);
    }

    char label_buffer[10];
    snprintf(label_buffer, 10, "<%s,%c,%c>",
        rexdd_rule_name[e.label.rule],
        e.label.complemented ? 'c' : '_',
        e.label.swapped ? 's' : '_'
    );
    fprintf(f,"\t0 [shape = point]\n");
    if (!rexdd_is_terminal(e.target)){
        fprintf(f, "\t0 -> \"%llu\" [style = solid label = \"%s\"]\n",e.target, label_buffer);
        fprint_rexdd (f, F, e);
    } else {
        fprintf(f, "\t0 -> \"T0\" [style = solid label = \"%s\"]\n", label_buffer);
    }
    
    fprintf(f, "\t{rank=same v0 \"T0\" [label = \"0\", shape = square]}\n");

    fprintf(f,"}");
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

    rexdd_default_forest_settings(3, &s);
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
    child.edge[0].target = rexdd_make_terminal(0);
    child.edge[0].label.rule = rexdd_rule_EH1;
    child.edge[0].label.complemented = 0;
    child.edge[0].label.swapped = 0;
    child.edge[1].target = rexdd_make_terminal(0);
    child.edge[1].label.rule = rexdd_rule_X;
    child.edge[1].label.complemented = 0;
    child.edge[1].label.swapped = 0;

    rexdd_node_handle_t h_child = rexdd_nodeman_get_handle(F.M, &child);

    n.level = Levels;
    // n.edge[0].target = h_child;
    n.edge[0].target = rexdd_make_terminal(0);
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
    
    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;

    // rexdd_reduce_node(&F, &n, &e);
    rexdd_reduce_edge(&F, Levels, l, n, &e);

    /* ==========================================================================
     *      Old version of printing rexdd
     * ==========================================================================*/
    // printf("\nreduced edge: \n");
    // char buffer[20];
    // rexdd_snprint_edge(buffer, 20, e);
    // printf("\t%s\n", buffer);
    // if (e.target != h) {
    //     if (!rexdd_is_terminal(e.target)) {
    //         printf("---------Normalized node-------\n");
    //         rexdd_unpacked_node_t node;
    //         rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F.M,e.target), &node);
    //         show_unpacked_node(&F, node);
    //         printf("-------------------------------\n");
    //     }
    // }

    /* ==========================================================================
     *      New version of printing rexdd
     * ==========================================================================*/
    printf("\nCreating RexDD.gv file for printing...\n");
    FILE *f;
    f = fopen("RexDD.gv", "w+");
    build_gv(f, &F, e);
    fclose(f);
    printf("Done!\n");
    printf("Please use command to creat pdf file for viewing:\n");
    printf("\t\"dot -Tpdf RexDD.gv > RexDD.pdf\"\n");

    printf("\n=================Checking eval function...===================\n");

    /* ==========================================================================
     *      Building the vars for the specific number of levels
     * ==========================================================================*/
    uint32_t num_level = Levels;

    // Build the vars for the specific number of levels
    bool vars[0x01 << num_level][num_level+1];
    // Build the function results for the specific number of levels
    bool functions[0x01 << num_level][0x01 << (0x01 << num_level)];

    int var = (0x01 << num_level) - 1;
    int function = (0x01 << (0x01 << num_level)) - 1;
    
    bool out;
    // for (uint32_t k=0; k<(0x01 << (0x01 << num_level)); k++){
    //     for (uint32_t i=0; i<(0x01 << num_level); i++) {
    //         vars[i][0] = 0;
    //         for (uint32_t j=1; j<(num_level+1); j++) {
    //             vars[i][j] = var & (0x01<<(j-1));
    //         }
    //         var = var-1;
    //         functions[i][k] = function & (0x01<<(i));
    //     }
    //     function = function-1;
    // }

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
