
#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void boolNodes(rexdd_forest_t *F, rexdd_node_handle_t handle, bool count[handle+1])
{
    if (rexdd_is_terminal(handle)) {
        count[0] = 0;
    } else {
        if (!count[handle]) {
            count[handle] = 1;
        }
        rexdd_node_handle_t handleL, handleH;
        handleL = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M,handle));
        handleH = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M,handle));

        boolNodes(F, handleL, count);
        boolNodes(F, handleH, count);
    }
}

void fprint_rexdd(FILE *f, rexdd_forest_t *F, rexdd_edge_t e)
{
    char label_bufferL[10];
    char label_bufferH[10];
    bool countNode[e.target+1];
    for (uint64_t i=0; i<=e.target+1; i++) {
        countNode[i] = 0;
    }

    boolNodes(F, e.target, countNode);

    for (uint64_t i=0; i<=e.target+1; i++) {
        if (countNode[i]){
            fprintf(f, "\t{rank=same v%d N%llu [label = \"N%llu_%llu\", shape = circle]}\n",
                        rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, i)),
                        i,
                        i / REXDD_PAGE_SIZE,
                        i % REXDD_PAGE_SIZE);

            rexdd_unpacked_node_t node;
            rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, i), &node);
            snprintf(label_bufferL, 10, "<%s,%c,%c>",
                        rexdd_rule_name[node.edge[0].label.rule],
                        node.edge[0].label.complemented ? 'c' : '_',
                        node.edge[0].label.swapped ? 's' : '_');
            snprintf(label_bufferH, 10, "<%s,%c,%c>",
                        rexdd_rule_name[node.edge[1].label.rule],
                        node.edge[1].label.complemented ? 'c' : '_',
                        node.edge[1].label.swapped ? 's' : '_');
            if (rexdd_is_terminal(node.edge[0].target)) {
                fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = dashed label = \"%s\"]\n",
                        i,
                        rexdd_terminal_value(node.edge[0].target),
                        label_bufferL);
            } else {
                fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = dashed label = \"%s\"]\n",
                        i,
                        node.edge[0].target,
                        label_bufferL);
            }
            if (rexdd_is_terminal(node.edge[1].target)) {
                fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = solid label = \"%s\"]\n",
                        i,
                        rexdd_terminal_value(node.edge[1].target),
                        label_bufferH);
            } else {
                fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = solid label = \"%s\"]\n",
                        i,
                        node.edge[1].target,
                        label_bufferH);
            }
        } else {
            continue;
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
        fprintf(f, "\t0 -> \"N%llu\" [style = solid label = \"%s\"]\n", e.target, label_buffer);
        fprint_rexdd(f, F, e);
    }
    else
    {
        fprintf(f, "\t0 -> \"T0\" [style = solid label = \"%s\"]\n", label_buffer);
    }

    fprintf(f, "\t{rank=same v0 \"T0\" [label = \"0\", shape = square]}\n");

    fprintf(f, "}");
}

void build_gv_forest(FILE *f, rexdd_forest_t *F, rexdd_edge_t ptr[], int size)
{
    fprintf(f, "digraph g\n{\n");
    fprintf(f, "\trankdir=TB\n");

    fprintf(f, "\n\tnode [shape = none]\n");
    fprintf(f, "\tv0 [label=\"\"]\n");
    
    for (uint32_t i = 1; i <= F->S.num_levels; i++)
    {
        fprintf(f, "\tv%d [label=\"x%d\"]\n", i, i);
        fprintf(f, "\tv%d -> v%d [style=invis]\n", i, i - 1);
    }

    char label_buffer[10];
    for (int i=0; i<size; i++) {
        snprintf(label_buffer, 10, "<%s,%c,%c>",
                rexdd_rule_name[ptr[i].label.rule],
                ptr[i].label.complemented ? 'c' : '_',
                ptr[i].label.swapped ? 's' : '_');
        fprintf(f, "\t{rank=same v4 %d [shape = point]}\n", i);
        if (!rexdd_is_terminal(ptr[i].target))
        {
            fprintf(f, "\t%d -> \"N%llu\" [style = solid label = \"f_%d:%s\"]\n",
                    i, ptr[i].target, i, label_buffer);
            // fprint_rexdd(f, F, ptr[i]);
        }
        else
        {
            fprintf(f, "\t%d -> \"T0\" [style = solid label = \"f_%d:%s\"]\n",
                    i, i, label_buffer);
        }
    }

    rexdd_unpacked_node_t node;
    char label_bufferL[10];
    char label_bufferH[10];
    for (rexdd_node_handle_t t=1; t<62; t++) {
        rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, t), &node);

        fprintf(f, "\t{rank=same v%d N%llu [label = \"N%llu_%llu\", shape = circle]}\n",
                rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, t)),
                t,
                t / REXDD_PAGE_SIZE,
                t % REXDD_PAGE_SIZE);
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
            fprintf(f, "\t\"N%llu\" -> \"T0\" [style = dashed label = \"%s\"]\n",
                    t,
                    label_bufferL);
            fprintf(f, "\t\"N%llu\" -> \"T0\" [style = solid label = \"%s\"]\n",
                    t,
                    label_bufferH);
        }
        else if (!rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target))
        {
            fprintf(f, "\t\"N%llu\" -> \"T0\" [style = dashed label = \"%s\"]\n",
                    t,
                    label_bufferL);
            fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = solid label = \"%s\"]\n",
                    t,
                    node.edge[1].target,
                    label_bufferH);
        }
        else if (rexdd_is_terminal(node.edge[1].target) && !rexdd_is_terminal(node.edge[0].target))
        {
            fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = dashed label = \"%s\"]\n",
                    t,
                    node.edge[0].target,
                    label_bufferL);
            fprintf(f, "\t\"N%llu\" -> \"T0\" [style = solid label = \"%s\"]\n",
                    t,
                    label_bufferH);
        }
        else
        {
            fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = dashed label = \"%s\"]\n",
                    t,
                    node.edge[0].target,
                    label_bufferL);
            fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = solid label = \"%s\"]\n",
                    t,
                    node.edge[1].target,
                    label_bufferH);
        }        
    }

    fprintf(f, "\t{rank=same v0 \"T0\" [label = \"0\", shape = square]}\n");

    fprintf(f, "}");

}

int countNodes(rexdd_forest_t *F, rexdd_node_handle_t handle)
{
    int count = 0;
    if (rexdd_is_terminal(handle)) {
        count = 0;
    } else {
        bool countNode[handle+1];
        for (uint64_t i=0; i<=handle+1; i++) {
            countNode[i] = 0;
        }
        boolNodes(F, handle, countNode);
        for (uint64_t i=0; i<=handle+1; i++) {
            count = count + countNode[i];
        }
    }
    return count;
}

void check_eval(rexdd_forest_t F, int levels, bool Vars_in[][levels], bool Function_in[][0x01 << (0x01 << (levels-1))], rexdd_edge_t ptr_in[], 
                                            bool Vars_out[][levels+1], bool Function_out[][0x01 << (0x01 << levels)], rexdd_edge_t ptr_out[])
{
    for (int i = 0; i < 0x01<<levels; i++)
    {
        Vars_out[i][0] = 0;
        if (i < 0x01<<(levels-1))
        {
            Vars_out[i][levels] = 0;
        }
        else
        {
            Vars_out[i][levels] = 1;
        }
        for (int j = 1; j < levels; j++)
        {
            Vars_out[i][j] = Vars_in[i % (0x01<<(levels-1))][j];
        }
        for (int k = 0; k<0x01<<(0x01<<levels); k++)
        {
            if (i<0x01<<(levels-1)) {
                Function_out[i][k] = Function_in[i%(0x01<<(levels-1))][k/(0x01<<(0x01<<(levels-1)))];
            } else {
                Function_out[i][k] = Function_in[i%(0x01<<(levels-1))][k%(0x01<<(0x01<<(levels-1)))];
            }
        }
    }
    rexdd_unpacked_node_t temp;
    temp.level = 1;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;

    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;

    temp.level = levels;

    printf("check eval for level %d...\n", levels);
    for (int t=0; t<0x01<<(0x01<<levels); t++) {
        temp.edge[0] = ptr_in[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr_in[t % (0x01<<(0x01<<(levels-1)))];
        
        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        // char buffer[128];
        // printf("build dot file\n");
        // snprintf(buffer, 12*levels, "L%d_%d.gv",levels, t);
        // FILE *f_temp;
        // f_temp = fopen(buffer, "w+");
        // build_gv(f_temp, &F, eval);
        // fclose(f_temp);

        ptr_out[t] = eval;

        for (int i=0; i<0x01<<levels; i++){
            if (rexdd_eval(&F, &eval, levels, Vars_out[i]) == Function_out[i][t])
            {
                continue;
            }
            else
            {
                for (int j = 0; j < 0x01<<levels; j++)
                {
                    printf("\t%d", Function_out[j][t]);
                }
                printf("\n");
                rexdd_check1(0, "Eval error!");
                break;
            }
        }
    }
    printf("Done eval!\n");
}

void export_funsNum(rexdd_forest_t F, int levels, rexdd_edge_t edges[])
{
    printf("Counting number of nodes...\n");
    FILE *f1, *f2;
    char buffer1[16], buffer2[16];
    snprintf(buffer1, 16, "L%d_nodeFun.txt", levels);
    snprintf(buffer2, 16, "L%d_funNode.txt", levels);
    f1 = fopen(buffer1, "w+");
    f2 = fopen(buffer2, "w+");

    int max_num = 0;
    for (int i=0; i<0x01<<(0x01<<levels); i++) {
        if (max_num < countNodes(&F, edges[i].target)) {
            max_num = countNodes(&F, edges[i].target);
        }
        fprintf(f2, "%d %d\n", i, countNodes(&F, edges[i].target) + 1);
    }
    printf("max number of nodes(including terminal) for any level %d QBDD is: %d\n\n",levels, max_num+1);

    int numFunc[max_num+1];
    for (int i=0; i<max_num+1; i++) {
        numFunc[i] = 0;
    }

    for (int i=0; i<0x01<<(0x01<<levels); i++) {
        numFunc[countNodes(&F,edges[i].target)]++;
    }

    for (int i=0; i<max_num+1; i++) {
        fprintf(f1, "%d %d\n", i+1, numFunc[i]);
    }
    fclose(f1);
    fclose(f2);
    printf("Done counting!\n\n");
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

    rexdd_unpacked_node_t temp;
    temp.level = 1;
    temp.edge[0].target = rexdd_make_terminal(0);
    temp.edge[1].target = rexdd_make_terminal(0);
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;

    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;

    int levels;

    /* ==========================================================================
     *      Building the vars for level one nodes
     * ==========================================================================*/
    levels = 1;
    printf("Testing Level One...\n");

    bool Vars_1[0x01 << levels][levels + 1];
    bool Function_1[0x01 << levels][0x01 << (0x01 << levels)];
    rexdd_edge_t ptr1[0x01<<(0x01<<levels)];

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

    printf("check eval for level %d...\n",levels);
    for (int k = 0; k < 0x01 << (0x01 << levels); k++)
    {
        temp.edge[Vars_1[0][1]].label.complemented = Function_1[0][k];
        temp.edge[Vars_1[1][1]].label.complemented = Function_1[1][k];

        rexdd_edge_t eval;
        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        // char buffer[128];
        // printf("build dot file\n");
        // snprintf(buffer, 12*levels, "L%d_%d.gv",levels, t);
        // FILE *f_temp;
        // f_temp = fopen(buffer, "w+");
        // build_gv(f_temp, &F, eval);
        // fclose(f_temp);

        ptr1[k] = eval;

        for (int i = 0; i < 0x01 << levels; i++)
        {
            if (rexdd_eval(&F, &eval, 1, Vars_1[i]) == Function_1[i][k])
            {
                continue;
            }
            else
            {
                for (int j = 0; j < 0x01 << levels; j++)
                {
                    printf("\t%d", Function_1[j][k]);
                }
                printf("\n");
                rexdd_check1(0, "Eval error!");
                break;
            }
        }
    }
    printf("Done!\n\n");
    export_funsNum(F, levels, ptr1);

    /* ==========================================================================
     *      Building the vars for level two nodes
     * ==========================================================================*/
    levels = 2;
    printf("Testing Level Two...\n");
    bool Vars_2[0x01<<levels][levels+1];
    bool Function_2[0x01<<levels][0x01<<(0x01<<levels)];
    rexdd_edge_t ptr2[0x01<<(0x01<<levels)];

    check_eval(F, 2, Vars_1, Function_1, ptr1, Vars_2, Function_2, ptr2);
    export_funsNum(F, levels, ptr2);

    /* ==========================================================================
     *      Building the vars for level three nodes
     * ==========================================================================*/
    levels = 3;
    printf("Testing Level Three...\n");
    bool Vars_3[0x01<<levels][levels+1];
    bool Function_3[0x01<<levels][0x01<<(0x01<<levels)];
    rexdd_edge_t ptr3[0x01<<(0x01<<levels)];

    check_eval(F, 3, Vars_2, Function_2, ptr2, Vars_3, Function_3, ptr3);
    export_funsNum(F, levels, ptr3);

    /* ==========================================================================
     *      Building the vars for level four nodes
     * ==========================================================================*/
    levels = 4;
    printf("Testing Level Four...\n");
    bool Vars_4[0x01<<levels][levels+1];
    bool Function_4[0x01<<levels][0x01<<(0x01<<levels)];
    rexdd_edge_t ptr4[0x01<<(0x01<<levels)];

    check_eval(F, 4, Vars_3, Function_3, ptr3, Vars_4, Function_4, ptr4);
    export_funsNum(F, levels, ptr4);

    printf("\n=============================================================\n");

    printf("first unallocate handle: %u\n", F.M->pages->first_unalloc);

    int node_l;
    int count_nodeLvl[levels];
    for (int i=0; i<levels; i++) {
        count_nodeLvl[i] = 0;
    }
    for (rexdd_node_handle_t h = 1; h<=F.M->pages->first_unalloc; h++) {
        if (!rexdd_is_terminal(h)) {
            node_l = rexdd_unpack_level(rexdd_get_packed_for_handle(F.M, h));
            count_nodeLvl[node_l-1]++;
        } else {
            continue;
        }
    }

    printf("number of nodes at each level in the forest\n");
    for (int i=1; i<5; i++) {
        printf("L%d: %d\n", i, count_nodeLvl[i-1]);
    }

    // ---------------------------------Example---------------------------------
    FILE *t;
    t = fopen("test.gv", "w+");
    build_gv(t, &F, ptr4[355]);
    fclose(t);
    // -------------------------------------------------------------------------

    rexdd_free_forest(&F);

    FILE *out;
    out = fopen("Vars.txt", "w+");
    fprintf(out, "x1\tx2\tx3\n");
    for (int i=0; i<2; i++) {
        fprintf(out, "%d\n", Vars_1[i][1]);
    }
    fprintf(out,"=============================\n");
    for (int i=0; i<4; i++) {
        fprintf(out, "%d\t%d\n", Vars_2[i][1],Vars_2[i][2]);
    }
    fprintf(out,"=============================\n");
    for (int i=0; i<8; i++) {
        fprintf(out, "%d\t%d\t%d\n", Vars_3[i][1],Vars_3[i][2],Vars_3[i][3]);
    }
    fprintf(out,"=============================\n");
    fclose(out);

    int lvl=1;
    out = fopen("Functions.txt", "w+");
    for (int i = 0; i < 0x01<<(0x01<<lvl); i++) {
        fprintf(out, "%d\t", i);
        for (int j = 0; j < 0x01<<lvl; j++)
        {
            fprintf(out, " %d ", Function_1[j][i]);
        }
        fprintf(out,"\n");
    }
    fprintf(out,"=============================\n");
    lvl=2;
    for (int i = 0; i < 0x01<<(0x01<<lvl); i++) {
        fprintf(out, "%d\t", i);
        for (int j = 0; j < 0x01<<lvl; j++)
        {
            fprintf(out, " %d ", Function_2[j][i]);
        }
        fprintf(out,"\n");
    }
    fprintf(out,"=============================\n");
    lvl=3;
    for (int i = 0; i < 0x01<<(0x01<<lvl); i++) {
        fprintf(out, "%d\t", i);
        for (int j = 0; j < 0x01<<lvl; j++)
        {
            fprintf(out, " %d ", Function_3[j][i]);
        }
        fprintf(out,"\n");
    }
    fprintf(out,"=============================\n");
    lvl=4;
    for (int i = 0; i < 0x01<<(0x01<<lvl); i++) {
        fprintf(out, "%d\t", i);
        for (int j = 0; j < 0x01<<lvl; j++)
        {
            fprintf(out, " %d ", Function_4[j][i]);
        }
        fprintf(out,"\n");
    }
    fprintf(out,"=============================\n");
    
    fclose(out);
    
    return 0;
}
