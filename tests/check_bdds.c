#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define TEST_QBDD

// #define TEST_FBDD

#define TEST_ZBDD

void fprint_rexdd(FILE *f, rexdd_forest_t *F, rexdd_edge_t e)
{
    char label_bufferL[10];
    char label_bufferH[10];

    rexdd_unpacked_node_t node;
    rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, e.target), &node);
    fprintf(f, "\t{rank=same v%d N%llu [label = \"N%llu_%llu\", shape = circle]}\n",
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
        fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = dashed label = \"%s\"]\n",
                e.target,
                rexdd_terminal_value(node.edge[0].target),
                label_bufferL);
        fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = solid label = \"%s\"]\n",
                e.target,
                rexdd_terminal_value(node.edge[1].target),
                label_bufferH);
    }
    else if (!rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target))
    {
        fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = dashed label = \"%s\"]\n",
                e.target,
                rexdd_terminal_value(node.edge[0].target),
                label_bufferL);
        fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = solid label = \"%s\"]\n",
                e.target,
                node.edge[1].target,
                label_bufferH);
        fprint_rexdd(f, F, node.edge[1]);
    }
    else if (rexdd_is_terminal(node.edge[1].target) && !rexdd_is_terminal(node.edge[0].target))
    {
        fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = dashed label = \"%s\"]\n",
                e.target,
                node.edge[0].target,
                label_bufferL);
        fprint_rexdd(f, F, node.edge[0]);
        fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = solid label = \"%s\"]\n",
                e.target,
                rexdd_terminal_value(node.edge[1].target),
                label_bufferH);
    }
    else
    {
        fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = dashed label = \"%s\"]\n",
                e.target,
                node.edge[0].target,
                label_bufferL);
        fprintf(f, "\t\"N%llu\" -> \"N%llu\" [style = solid label = \"%s\"]\n",
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
        fprintf(f, "\t0 -> \"N%llu\" [style = solid label = \"%s\"]\n", e.target, label_buffer);
        fprint_rexdd(f, F, e);
    }
    else
    {
        fprintf(f, "\t0 -> \"T%llu\" [style = solid label = \"%s\"]\n",rexdd_terminal_value(e.target), label_buffer);
    }

    fprintf(f, "\t{rank=same v0 \"T0\" [label = \"0\", shape = square]}\n");
    fprintf(f, "\t{rank=same v0 \"T1\" [label = \"1\", shape = square]}\n");


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
            if (rexdd_terminal_value(ptr[i].target)==0) {
                fprintf(f, "\t%d -> \"T0\" [style = solid label = \"f_%d:%s\"]\n",
                    i, i, label_buffer);
            } else {
                fprintf(f, "\t%d -> \"T1\" [style = solid label = \"f_%d:%s\"]\n",
                    i, i, label_buffer);
            }
            
        }
    }

    rexdd_unpacked_node_t node;
    char label_bufferL[10];
    char label_bufferH[10];
    for (rexdd_node_handle_t t=1; t<=F->M->pages->first_unalloc; t++) {
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
            fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = dashed label = \"%s\"]\n",
                    t,
                    rexdd_terminal_value(node.edge[0].target),
                    label_bufferL);
            fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = solid label = \"%s\"]\n",
                    t,
                    rexdd_terminal_value(node.edge[1].target),
                    label_bufferH);
        }
        else if (!rexdd_is_terminal(node.edge[1].target) && rexdd_is_terminal(node.edge[0].target))
        {
            fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = dashed label = \"%s\"]\n",
                    t,
                    rexdd_terminal_value(node.edge[0].target),
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
            fprintf(f, "\t\"N%llu\" -> \"T%llu\" [style = solid label = \"%s\"]\n",
                    t,
                    rexdd_terminal_value(node.edge[1].target),
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
    fprintf(f, "\t{rank=same v0 \"T1\" [label = \"1\", shape = square]}\n");


    fprintf(f, "}");

}

int countNodes(rexdd_forest_t *F, rexdd_node_handle_t handle)
{
    int count = 0;
    if (rexdd_is_terminal(handle)) {
        return count;
    } else {
        count = 1;
        rexdd_node_handle_t handleL, handleH;
        handleL = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M,handle));
        handleH = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M,handle));

        if (!rexdd_is_terminal(handleL) && !rexdd_is_terminal(handleH)) {

            if (handleL != handleH) {
                    count = count + countNodes(F, handleL) + countNodes(F, handleH);
            } else {
                count = count + countNodes(F, handleL);
            }
            
        } else if (!rexdd_is_terminal(handleL) && rexdd_is_terminal(handleH)) {
                count = count + countNodes(F, handleL);
        } else if (rexdd_is_terminal(handleL) && !rexdd_is_terminal(handleH)) {
                count = count + countNodes(F, handleH);
        } else {
            return count;
        }
        return count;
    }
}

int countTerm(rexdd_forest_t *F, rexdd_node_handle_t handle)
{
    int count = 0;
    if (rexdd_is_terminal(handle)) {
        count = rexdd_terminal_value(handle);
    } else {
        rexdd_node_handle_t handleL, handleH;
        handleL = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M,handle));
        handleH = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M,handle));
        if (countTerm(F, handleL) != countTerm(F,handleH)) {
            count = 2;
        } else {
            count = countTerm(F,handleL);
        }
    }
    return count;
}

#ifdef TEST_FBDD
void fbdd_reduce_edge(rexdd_forest_t *F, rexdd_unpacked_node_t temp, rexdd_edge_t *out)
{
    out->label.rule = rexdd_rule_X;
    out->label.complemented = 0;
    out->label.swapped = 0;
    if (temp.edge[0].target == temp.edge[1].target) {
        out->target = temp.edge[0].target;
    } else {
        out->target = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, &temp));
    }
}
#endif

#ifdef TEST_ZBDD
void zbdd_reduce_edge(rexdd_forest_t *F, rexdd_unpacked_node_t temp, rexdd_edge_t *out)
{
    out->label.rule = rexdd_rule_X;
    out->label.complemented = 0;
    out->label.swapped = 0;
    if (temp.edge[1].target == rexdd_make_terminal(0)) {
        out->target = temp.edge[0].target;
    } else {
        out->target = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M, &temp));
    }
}

bool zbdd_eval(rexdd_forest_t *F, rexdd_edge_t *e, uint32_t m, bool *vars)
{
    bool result = 0;
    
    uint32_t k;
    if (rexdd_is_terminal(e->target)) {
        k = 0;
    } else {
        k = rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e->target));
    }

    if (m == k) {
        if (m == 0) {
            result = rexdd_terminal_value(e->target);
        } else {
            rexdd_unpacked_node_t temp;
            rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F->M, e->target), &temp);
            if (vars[k]) {
                result = zbdd_eval(F, &temp.edge[1], k-1, vars);
            } else {
                result = zbdd_eval(F, &temp.edge[0], k-1, vars);
            }
        }
    } else {
        bool flag = 0;
        for (uint32_t i=k+1; i<=m; i++) {
            flag = flag | vars[i];
        }
        if (flag) {
            result = 0;
        } else {
            result = zbdd_eval(F, e, k, vars);
        }
    }

    return result;
}
#endif

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

    rexdd_edge_t eval;
    eval.label.rule = rexdd_rule_X;
    eval.label.complemented = 0;
    eval.label.swapped = 0;

    temp.level = levels;

    printf("check eval for level %d...\n", levels);
    for (int t=0; t<0x01<<(0x01<<levels); t++) {
        temp.edge[0] = ptr_in[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr_in[t % (0x01<<(0x01<<(levels-1)))];
#ifdef TEST_QBDD
        eval.target = rexdd_insert_UT(F.UT, rexdd_nodeman_get_handle(F.M, &temp));
#endif

#ifdef TEST_FBDD
        fbdd_reduce_edge(&F, temp, &eval);
#endif

#ifdef TEST_ZBDD
        zbdd_reduce_edge(&F, temp, &eval);
#endif

        ptr_out[t] = eval;

        for (int i=0; i<0x01<<levels; i++){
#ifndef TEST_ZBDD
            if (rexdd_eval(&F, &eval, levels, Vars_out[i]) == Function_out[i][t])
            {
                continue;
            }
#endif

#ifdef TEST_ZBDD
            if (zbdd_eval(&F, &eval, levels, Vars_out[i]) == Function_out[i][t])
            {
                continue;
            }
#endif
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
    printf("Done!\n\n");
}

void export_funsNum(rexdd_forest_t F, int levels, rexdd_edge_t edges[])
{
    FILE *test;
    char buffer[20];
#ifdef TEST_QBDD
    snprintf(buffer, 20, "L%d_nodeFun_QBDD.txt", levels);
#endif

#ifdef TEST_FBDD
    snprintf(buffer, 20, "L%d_nodeFun_FBDD.txt", levels);
#endif

#ifdef TEST_ZBDD
    snprintf(buffer, 20, "L%d_nodeFun_ZBDD.txt", levels);
#endif
    test = fopen(buffer, "w+");

    int max_num = 0;
    int num_term = 0;
    for (int i=0; i<0x01<<(0x01<<levels); i++) {
        if (countTerm(&F,edges[i].target) == 0 || countTerm(&F,edges[i].target) == 1) {
            num_term = 1;
        } else {
            num_term = countTerm(&F,edges[i].target);
        }
        if (max_num < (countNodes(&F, edges[i].target) + num_term)) {
            max_num = countNodes(&F, edges[i].target) + num_term;
        }
    }
    printf("max number of nodes(including terminal) for any level %d QBDD is: %d\n\n",levels, max_num);

    int numFunc[max_num+1];
    for (int i=0; i<max_num+1; i++) {
        numFunc[i] = 0;
    }

    for (int i=0; i<0x01<<(0x01<<levels); i++) {
        if (countTerm(&F,edges[i].target) == 0 || countTerm(&F,edges[i].target) == 1) {
            num_term = 1;
        } else {
            num_term = countTerm(&F,edges[i].target);
        }
        numFunc[countNodes(&F,edges[i].target) + num_term]++;
    }

    for (int i=1; i<max_num+1; i++) {
        fprintf(test, "%d %d\n", i, numFunc[i]);
    }
    fclose(test);
}


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
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;

    rexdd_edge_t eval;
    eval.label.rule = rexdd_rule_X;
    eval.label.complemented = 0;
    eval.label.swapped = 0;

    int levels;
    /* ==========================================================================
     *      Testing eval function in QBDD case
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
    for (int k = 0; k < 0x01 << (0x01 << levels); k++) {
        temp.edge[Vars_1[0][1]].target = rexdd_make_terminal(Function_1[0][k]);
        temp.edge[Vars_1[1][1]].target = rexdd_make_terminal(Function_1[1][k]);
#ifdef TEST_QBDD
        eval.target = rexdd_insert_UT(F.UT, rexdd_nodeman_get_handle(F.M, &temp));
#endif

#ifdef TEST_FBDD
        fbdd_reduce_edge(&F, temp, &eval);
#endif

#ifdef TEST_ZBDD
        zbdd_reduce_edge(&F, temp, &eval);
#endif

        ptr1[k] = eval;

        for (int i = 0; i < 0x01 << levels; i++)
        {
#ifndef TEST_ZBDD
            if (rexdd_eval(&F, &eval, levels, Vars_1[i]) == Function_1[i][k])
            {
                continue;
            }
#endif

#ifdef TEST_ZBDD
            if (zbdd_eval(&F, &eval, levels, Vars_1[i]) == Function_1[i][k])
            {
                continue;
            }
#endif
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

    FILE *f;
#ifdef TEST_QBDD
    f = fopen("QBDD.gv", "w+");
#endif

#ifdef TEST_FBDD
    f = fopen("FBDD.gv", "w+");
#endif

#ifdef TEST_ZBDD
    f = fopen("ZBDD.gv", "w+");
#endif
    build_gv_forest(f, &F, ptr2, 16);
    fclose(f);

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
    for (uint_fast32_t i=1; i<F.S.num_levels; i++) {
        printf("L%d: %d\n", i, count_nodeLvl[i-1]);
    }

    rexdd_free_forest(&F);
    

    return 0;
}