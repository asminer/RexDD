
#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <dirent.h>

#define CHECK_PATTERN

void show_unpacked_node(rexdd_forest_t *F, rexdd_unpacked_node_t n)
{
    char buffer[20];

    printf("\nLevel %d node's low \n", n.level);
    rexdd_snprint_edge(buffer, 20, n.edge[0]);
    printf("\t%s\n", buffer);
    if (!rexdd_is_terminal(n.edge[0].target))
    {
        printf("---------Low child-------------\n");
        rexdd_unpacked_node_t nodel;
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, n.edge[0].target), &nodel);
        show_unpacked_node(F, nodel);
        printf("-------------------------------\n");
    }

    printf("\nLevel %d node's high \n", n.level);
    rexdd_snprint_edge(buffer, 20, n.edge[1]);
    printf("\t%s\n", buffer);
    if (!rexdd_is_terminal(n.edge[1].target))
    {
        printf("---------High child-------------\n");
        rexdd_unpacked_node_t nodeh;
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, n.edge[1].target), &nodeh);
        show_unpacked_node(F, nodeh);
        printf("-------------------------------\n");
    }
}

void fprint_rexdd(FILE *f, rexdd_forest_t *F, rexdd_edge_t e)
{
    char label_bufferL[10];
    char label_bufferH[10];

    rexdd_unpacked_node_t node;
    rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, e.target), &node);
    fprintf(f, "\t{rank=same v%d %llu [label = \"N%llu_%llu\", shape = circle]}\n",
            rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e.target)),
            e.target,
            e.target / REXDD_PAGE_SIZE,
            e.target % REXDD_PAGE_SIZE);

    snprintf(label_bufferL, 10, "<%s,%c,%c>",
             rexdd_rule_name[node.edge[0].label.rule],
             node.edge[0].label.complemented ? 'c' : '_',
             node.edge[0].label.swapped ? 's' : '_');
    snprintf(label_bufferH, 10, "<%s,%c,%c>",
             rexdd_rule_name[node.edge[1].label.rule],
             node.edge[1].label.complemented ? 'c' : '_',
             node.edge[1].label.swapped ? 's' : '_');
    if (rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target))
    {
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = dashed label = \"%s\"]\n",
                e.target,
                label_bufferL);
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = solid label = \"%s\"]\n",
                e.target,
                label_bufferH);
    }
    else if (!rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target))
    {
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = dashed label = \"%s\"]\n",
                e.target,
                label_bufferL);
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = solid label = \"%s\"]\n",
                e.target,
                node.edge[1].target,
                label_bufferH);
        fprint_rexdd(f, F, node.edge[1]);
    }
    else if (rexdd_is_terminal(node.edge[1].target) && !rexdd_is_terminal(node.edge[0].target))
    {
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = dashed label = \"%s\"]\n",
                e.target,
                node.edge[0].target,
                label_bufferL);
        fprint_rexdd(f, F, node.edge[0]);
        fprintf(f, "\t\"%llu\" -> \"T0\" [style = solid label = \"%s\"]\n",
                e.target,
                label_bufferH);
    }
    else
    {
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = dashed label = \"%s\"]\n",
                e.target,
                node.edge[0].target,
                label_bufferL);
        fprintf(f, "\t\"%llu\" -> \"%llu\" [style = solid label = \"%s\"]\n",
                e.target,
                node.edge[1].target,
                label_bufferH);
        if (node.edge[0].target != node.edge[1].target)
        {
            fprint_rexdd(f, F, node.edge[0]);
            fprint_rexdd(f, F, node.edge[1]);
        }
        else
        {
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
    if (!rexdd_is_terminal(e.target))
    {
        for (uint32_t i = 1; i <= rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e.target)); i++)
        {
            fprintf(f, "\tv%d [label=\"x%d\"]\n", i, i);
            fprintf(f, "\tv%d -> v%d [style=invis]\n", i, i - 1);
        }
    }

    char label_buffer[10];
    snprintf(label_buffer, 10, "<%s,%c,%c>",
             rexdd_rule_name[e.label.rule],
             e.label.complemented ? 'c' : '_',
             e.label.swapped ? 's' : '_');
    fprintf(f, "\t0 [shape = point]\n");
    if (!rexdd_is_terminal(e.target))
    {
        fprintf(f, "\t0 -> \"%llu\" [style = solid label = \"%s\"]\n", e.target, label_buffer);
        fprint_rexdd(f, F, e);
    }
    else
    {
        fprintf(f, "\t0 -> \"T0\" [style = solid label = \"%s\"]\n", label_buffer);
    }

    fprintf(f, "\t{rank=same v0 \"T0\" [label = \"0\", shape = square]}\n");

    fprintf(f, "}");
}

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

    printf("=================Simple pattern checking test==================\n");

    /* ==========================================================================
     *      Building the unpacked node and the number of levels
     *          TBD----Build more and different nodes and edges
     * ==========================================================================*/
    uint32_t Levels = 3;

    child.level = Levels - 1;
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
    // uint32_t num_level = Levels;

    // // Build the vars for the specific number of levels
    // bool vars[0x01 << num_level][num_level+1];
    // int var = (0x01 << num_level) - 1;

    // bool out;
    // for (uint32_t i=0; i<(0x01 << num_level); i++) {
    //     vars[i][0] = 0;
    //     for (uint32_t j=1; j<(num_level+1); j++) {
    //         vars[i][j] = var & (0x01<<(j-1));
    //         printf("%d\t", vars[i][j]);
    //     }

    //     // Checking the eval function for the target edge
    //     out = rexdd_eval(&F, &e, Levels, vars[i]);
    //     printf("eval out: %d\n", out);
    //     printf("\n");
    //     var = var-1;
    // }
    printf("Testing Level One...\n");

    bool Vars_1[2][2];
    bool Function_1[2][4];
    int Var_1 = (0x01 << 1) - 1;
    int Func_1 = (0x01 << (0x01 << 1)) - 1;
    for (int k = 0; k < 4; k++)
    {
        for (int i = 0; i < 2; i++)
        {
            Vars_1[i][0] = 0;
            for (int j = 1; j < 2; j++)
            {
                Vars_1[i][j] = Var_1 & (0x01 << (j - 1));
            }
            Var_1 = Var_1 - 1;
            Function_1[i][k] = Func_1 & (0x01 << (i));
        }
        Func_1 = Func_1 - 1;
    }

    rexdd_unpacked_node_t temp;
    temp.level = 1;

    if (temp.level == 1)
    {
        temp.edge[0].target = rexdd_make_terminal(0);
        temp.edge[1].target = rexdd_make_terminal(0);
        temp.edge[0].label.swapped = 0;
        temp.edge[1].label.swapped = 0;
        temp.edge[0].label.rule = rexdd_rule_X;
        temp.edge[1].label.rule = rexdd_rule_X;
    }

    char buffer[16];
    rexdd_edge_t ptr[4];

    // function #k
    for (int k = 0; k < 4; k++)
    {
        temp.edge[Vars_1[0][1]].label.complemented = Function_1[0][k];
        temp.edge[Vars_1[1][1]].label.complemented = Function_1[1][k];
        // show_unpacked_node(&F, temp);

        printf("reduce edge...\n");
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, 1, l, temp, &eval);

        printf("build dot file\n");
        snprintf(buffer, 16, "Temp_%d.gv", k);
        FILE *f_temp;
        f_temp = fopen(buffer, "w+");
        build_gv(f_temp, &F, eval);
        fclose(f_temp);

        ptr[k] = eval;

        printf("check eval...\t%d\n",k);
        for (int i = 0; i < 2; i++)
        {
            if (rexdd_eval(&F, &eval, 1, Vars_1[i]) == Function_1[i][k])
            {
                continue;
            }
            else
            {
                printf("Eval error!\n");
                for (int j = 0; j < 2; j++)
                {
                    printf("\t%d", Function_1[j][k]);
                }
                printf("\n");
                break;
            }
        }
    }
    printf("\nDone!\n");

    printf("Testing Level Two...\n");
    bool Vars_2[4][3];
    bool Function_2[4][16];

    for (int i = 0; i < 4; i++)
    {
        Vars_2[i][0] = 0;
        if (i < 2)
        {
            Vars_2[i][2] = 0;
        }
        else
        {
            Vars_2[i][2] = 1;
        }
        for (int j = 1; j < 2; j++)
        {
            Vars_2[i][j] = Vars_1[i % 2][j];
        }
        for (int l = 0; l<16; l++)
        {
            if (i<2) {
                Function_2[i][l] = Function_1[i%2][l/4];
            } else {
                Function_2[i][l] = Function_1[i%2][l%4];
            }
        }
    }

    temp.level = 2;
    for (int t=0; t<16; t++) {
        temp.edge[0] = ptr[t/4];
        temp.edge[1] = ptr[t%4];
        
        printf("reduce edge...\n");
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, 2, l, temp, &eval);

        printf("build dot file\n");
        snprintf(buffer, 16, "Temp2_%d.gv", t);
        FILE *f_temp;
        f_temp = fopen(buffer, "w+");
        build_gv(f_temp, &F, eval);
        fclose(f_temp);

        printf("check eval...\t%d\n",t);
        for (int i=0; i<4; i++){
            if (rexdd_eval(&F, &eval, 2, Vars_2[i]) == Function_2[i][t])
            {
                continue;
            }
            else
            {
                printf("Eval error!\n");
                for (int j = 0; j < 4; j++)
                {
                    printf("\t%d", Function_2[j][t]);
                }
                printf("\n");
                break;
            }
        }

        printf("Done!\n");
    }
    for (int i=0; i<4; i++) {
        for (int j=1; j<3; j++) {
            printf("%d\t", Vars_2[i][j]);
        }
        printf("\n");
    }
    printf("\n=============================================================\n");

    rexdd_free_forest(&F);

    return 0;
}
