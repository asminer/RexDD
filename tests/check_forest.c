
#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

/* ================================================================================================ */

int main()
{
    /* ==========================================================================
     *      Initializing for testing
     * ==========================================================================*/
    rexdd_forest_t F;
    rexdd_forest_settings_t s;

    rexdd_default_forest_settings(5, &s);
    rexdd_init_forest(&F, &s);

    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;

    int levels;

    printf("\n=================Checking eval function...===================\n");

    /* ==========================================================================
     *      Building the vars for level one nodes
     * ==========================================================================*/
    levels = 1;
    printf("Testing Level One...\n");

    bool Vars_1[0x01 << levels][levels + 1];
    bool Function_1[0x01 << levels][0x01 << (0x01 << levels)];
    int Var_1 = (0x01 << levels) - 1;
    int Func_1 = (0x01 << (0x01 << levels)) - 1;

    for (int k = 0; k < 0x01 << (0x01 << levels); k++)
    {
        for (int i = 0; i < 0x01 << levels; i++)
        {
            Vars_1[i][0] = 0;
            for (int j = 1; j < 0x01 << levels; j++)
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

    char buffer[128];
    rexdd_edge_t ptr1[0x01<<(0x01<<levels)];
    FILE *f_temp;

    for (int k = 0; k < 0x01 << (0x01 << levels); k++)
    {
        temp.edge[Vars_1[0][1]].label.complemented = Function_1[0][k];
        temp.edge[Vars_1[1][1]].label.complemented = Function_1[1][k];

        printf("reduce edge...\n");
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        printf("build dot file\n");
        snprintf(buffer, 16, "L%d_%d.gv",levels, k);

        f_temp = fopen(buffer, "w+");
        build_gv(f_temp, &F, eval);
        fclose(f_temp);

        ptr1[k] = eval;

        printf("check eval...\t%d\n",k);
        for (int i = 0; i < 0x01 << levels; i++)
        {
            if (rexdd_eval(&F, &eval, 1, Vars_1[i]) == Function_1[i][k])
            {
                continue;
            }
            else
            {
                printf("Eval error!\n");
                for (int j = 0; j < 0x01 << levels; j++)
                {
                    printf("\t%d", Function_1[j][k]);
                }
                printf("\n");
                break;
            }
        }
    }
    printf("\nDone!\n");

    /* ==========================================================================
     *      Building the vars for level two nodes
     * ==========================================================================*/
    levels = 2;
    printf("Testing Level Two...\n");
    bool Vars_2[0x01<<levels][levels+1];
    bool Function_2[0x01<<levels][0x01<<(0x01<<levels)];
    rexdd_edge_t ptr2[0x01<<(0x01<<levels)];

    for (int i = 0; i < 0x01<<levels; i++)
    {
        Vars_2[i][0] = 0;
        if (i < 0x01<<(levels-1))
        {
            Vars_2[i][levels] = 0;
        }
        else
        {
            Vars_2[i][levels] = 1;
        }
        for (int j = 1; j < levels; j++)
        {
            Vars_2[i][j] = Vars_1[i % (0x01<<(levels-1))][j];
        }
        for (int k = 0; k<0x01<<(0x01<<levels); k++)
        {
            if (i<0x01<<(levels-1)) {
                Function_2[i][k] = Function_1[i%(0x01<<(levels-1))][k/(0x01<<(0x01<<(levels-1)))];
            } else {
                Function_2[i][k] = Function_1[i%(0x01<<(levels-1))][k%(0x01<<(0x01<<(levels-1)))];
            }
        }
    }

    temp.level = levels;
    for (int t=0; t<0x01<<(0x01<<levels); t++) {
        temp.edge[0] = ptr1[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr1[t % (0x01<<(0x01<<(levels-1)))];
        
        printf("reduce edge...\n");
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        printf("build dot file\n");
        snprintf(buffer, 16, "L%d_%d.gv",levels, t);
        // FILE *f_temp;
        f_temp = fopen(buffer, "w+");
        build_gv(f_temp, &F, eval);
        fclose(f_temp);

        ptr2[t] = eval;

        printf("check eval...\t%d\n",t);
        for (int i=0; i<0x01<<levels; i++){
            if (rexdd_eval(&F, &eval, levels, Vars_2[i]) == Function_2[i][t])
            {
                continue;
            }
            else
            {
                printf("Eval error!\n");
                for (int j = 0; j < 0x01<<levels; j++)
                {
                    printf("\t%d", Function_2[j][t]);
                }
                printf("\n");
                break;
            }
        }
    }
    printf("\nDone!\n");

    /* ==========================================================================
     *      Building the vars for level three nodes
     * ==========================================================================*/
    levels = 3;
    printf("Testing Level Three...\n");
    bool Vars_3[0x01<<levels][levels+1];
    bool Function_3[0x01<<levels][0x01<<(0x01<<levels)];
    rexdd_edge_t ptr3[0x01<<(0x01<<levels)];

    for (int i = 0; i < 0x01<<levels; i++)
    {
        Vars_3[i][0] = 0;
        if (i < 0x01<<(levels-1))
        {
            Vars_3[i][levels] = 0;
        }
        else
        {
            Vars_3[i][levels] = 1;
        }
        for (int j = 1; j < levels; j++)
        {
            Vars_3[i][j] = Vars_2[i % (0x01<<(levels-1))][j];
        }
        for (int k = 0; k<0x01<<(0x01<<levels); k++)
        {
            if (i<0x01<<(levels-1)) {
                Function_3[i][k] = Function_2[i%(0x01<<(levels-1))][k/(0x01<<(0x01<<(levels-1)))];
            } else {
                Function_3[i][k] = Function_2[i%(0x01<<(levels-1))][k%(0x01<<(0x01<<(levels-1)))];
            }
        }
    }

    temp.level = levels;
    for (int t=0; t<0x01<<(0x01<<levels); t++) {
        temp.edge[0] = ptr2[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr2[t % (0x01<<(0x01<<(levels-1)))];
        
        printf("reduce edge...\n");
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        printf("build dot file\n");
        snprintf(buffer, 32, "L%d_%d.gv",levels, t);
        // FILE *f_temp;
        f_temp = fopen(buffer, "w+");
        build_gv(f_temp, &F, eval);
        fclose(f_temp);

        ptr3[t] = eval;

        printf("check eval...\t%d\n",t);
        for (int i=0; i<0x01<<levels; i++){
            if (rexdd_eval(&F, &eval, levels, Vars_3[i]) == Function_3[i][t])
            {
                continue;
            }
            else
            {
                printf("Eval error!\n");
                for (int j = 0; j < 0x01<<levels; j++)
                {
                    printf("\t%d", Function_3[j][t]);
                }
                printf("\n");
                break;
            }
        }
    }
    printf("\nDone!\n");

    /* ==========================================================================
     *      Building the vars for level four nodes
     * ==========================================================================*/
    levels = 4;
    printf("Testing Level Four...\n");
    bool Vars_4[0x01<<levels][levels+1];
    bool Function_4[0x01<<levels][0x01<<(0x01<<levels)];
    rexdd_edge_t ptr4[0x01<<(0x01<<levels)];

    for (int i = 0; i < 0x01<<levels; i++)
    {
        Vars_3[i][0] = 0;
        if (i < 0x01<<(levels-1))
        {
            Vars_4[i][levels] = 0;
        }
        else
        {
            Vars_4[i][levels] = 1;
        }
        for (int j = 1; j < levels; j++)
        {
            Vars_4[i][j] = Vars_3[i % (0x01<<(levels-1))][j];
        }
        for (int k = 0; k<0x01<<(0x01<<levels); k++)
        {
            if (i<0x01<<(levels-1)) {
                Function_4[i][k] = Function_3[i%(0x01<<(levels-1))][k/(0x01<<(0x01<<(levels-1)))];
            } else {
                Function_4[i][k] = Function_3[i%(0x01<<(levels-1))][k%(0x01<<(0x01<<(levels-1)))];
            }
        }
    }

    temp.level = levels;
    for (int t=0; t<0x01<<(0x01<<levels); t++) {
        temp.edge[0] = ptr3[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr3[t % (0x01<<(0x01<<(levels-1)))];
        
        printf("reduce edge...\n");
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        printf("build dot file\n");
        snprintf(buffer, 48, "L4_%d.gv", t);
        // FILE *f_temp;
        f_temp = fopen(buffer, "w+");
        build_gv(f_temp, &F, eval);
        fclose(f_temp);

        ptr4[t] = eval;

        printf("check eval...\t%d\n",t);
        for (int i=0; i<0x01<<levels; i++){
            bool result = rexdd_eval(&F, &eval, levels, Vars_4[i]);
            if (result == Function_4[i][t])
            {
                continue;
            }
            else
            {
                printf("Eval error!\n");
                for (int j = 0; j < 0x01<<levels; j++)
                {
                    printf(" %d ", Function_4[j][t]);
                }
                printf("\n");
                break;
            }
        }
    }
    printf("\nDone!\n");

    printf("\n=============================================================\n");

    rexdd_free_forest(&F);

    return 0;
}
