#include "forest.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//===============================================================================
void make_varsFuns(int levels, bool Vars_in[][levels], bool Function_in[][0x01 << (0x01 << (levels-1))],
                     bool Vars_out[][levels+1], bool Function_out[][0x01 << (0x01 << levels)])
{
    for (int i = 0; i < 0x01<<levels; i++)
    {
        Vars_out[i][0] = 0;
        Vars_out[i][levels] = !(i < 0x01<<(levels-1));
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
}

int32_t low_funNum(int levels, bool Function[0x01 << levels], 
                    bool Function_low[][0x01 << (0x01 << (levels-1))])
{
    int32_t low=0;
    for (int32_t k=0; k<0x01<<(0x01<<(levels-1)); k++) {
        for (int32_t i=0; i<0x01 << (levels-1); i++){
            if (Function[i]==Function_low[i][k]){
                if (i==((0x01<<(levels-1))-1)) {
                    low = k;
                }
                continue;
            } else {
                break;
            }
        }
    }
    return low;
}

int32_t high_funNum(int levels, bool Function[0x01 << levels], 
                    bool Function_high[][0x01 << (0x01 << (levels-1))])
{
    int32_t high=0;
    for (int32_t k=0; k<0x01<<(0x01<<(levels-1)); k++) {
        for (int32_t i=0x01<<(levels-1); i<0x01<<(levels); i++){
            if (Function[i]==Function_high[i%(0x01<<(levels-1))][k]){
                if (i==((0x01<<levels)-1)) {
                    high = k;
                }
                continue;
            } else {
                break;
            }
        }
    }
    return high;
}

void check_eval(rexdd_forest_t F, int levels, bool Vars_in[][levels], bool Function_in[][0x01 << (0x01 << (levels-1))], rexdd_edge_t ptr_in[], 
                                            bool Vars_out[][levels+1], bool Function_out[][0x01 << (0x01 << levels)], rexdd_edge_t ptr_out[])
{
    make_varsFuns(levels, Vars_in, Function_in, Vars_out, Function_out);

    rexdd_unpacked_node_t temp;
    temp.level = 1;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;

    rexdd_edge_t eval;
    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;
    eval.label = l;

    temp.level = levels;

    printf("check eval for level %d...\n", levels);
    unsigned long t;
    for (t=0; t<0x01UL<<(0x01<<levels); t++) {
        temp.edge[0] = ptr_in[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr_in[t % (0x01<<(0x01<<(levels-1)))];

        rexdd_reduce_edge(&F, levels, l, temp, &eval);

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

//=================================================================================

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
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;
#ifndef REXBDD
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
#else
    temp.edge[0].target = rexdd_make_terminal(1);
    temp.edge[1].target = rexdd_make_terminal(1);
#endif

    rexdd_edge_t eval;
    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;
    eval.label = l;

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
#ifndef REXBDD
        temp.edge[Vars_1[0][1]].target = rexdd_make_terminal(Function_1[0][k]);
        temp.edge[Vars_1[1][1]].target = rexdd_make_terminal(Function_1[1][k]);
#else
        temp.edge[Vars_1[0][1]].label.complemented = !Function_1[0][k];
        temp.edge[Vars_1[1][1]].label.complemented = !Function_1[1][k];
#endif

        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        ptr1[k] = eval;

        for (int i = 0; i < 0x01 << levels; i++)
        {
            if (rexdd_eval(&F, &eval, levels, Vars_1[i]) == Function_1[i][k])
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
#ifdef QBDD
    f = fopen("QBDD.gv", "w+");
#endif
#ifdef C_QBDD
    f = fopen("CQBDD.gv", "w+");
#endif
#ifdef CS_QBDD
    f = fopen("CSQBDD.gv", "w+");
#endif

#ifdef FBDD
    f = fopen("FBDD.gv", "w+");
#endif
#ifdef C_FBDD
    f = fopen("CFBDD.gv", "w+");
#endif
#ifdef CS_FBDD
    f = fopen("CSFBDD.gv", "w+");
#endif

#ifdef ZBDD
    f = fopen("ZBDD.gv", "w+");
#endif

#ifdef ESRBDD
    f = fopen("ESRBDD.gv", "w+");
#endif
#ifdef CESRBDD
    f = fopen("CESRBDD.gv", "w+");
#endif

#ifdef REXBDD
    f = fopen("RexBDD.gv", "w+");
#endif
    // build_gv_forest(f, &F, ptr2, 16);
    // fclose(f);

    export_funsNum(F, levels, ptr2);

    // save(&F, ptr2, 0x01<<(0x01<<levels));

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

    /* ==========================================================================
     *      Building the vars for level five nodes
     * ==========================================================================*/
    levels = 5;
    printf("Testing Level Five...\n");
    int rows = 0x01<<levels;
    unsigned long cols = 0x01UL<<(0x01<<levels);
    bool Vars_5[rows][levels+1];
    
    // where to store the 2^32 root edges 64GB
    rexdd_edge_t *ptr5 = malloc(cols * sizeof(rexdd_edge_t));

    for (int i = 0; i < 0x01<<levels; i++)
    {
        Vars_5[i][0] = 0;
        Vars_5[i][levels] = !(i < 0x01<<(levels-1));
        for (int j = 1; j < levels; j++)
        {
            Vars_5[i][j] = Vars_4[i % (0x01<<(levels-1))][j];
        }
    }

    temp.level = 1;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;

    temp.level = levels;

    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;
    eval.label = l;


    printf("check eval for level %d...\n", levels);
    bool eval_real;
    unsigned long t;
//     uint64_t check_point = 0;

//     char gz[64], type_[8];

// #ifdef FBDD
//     snprintf(type_,8,"FBDD");
// #elif defined QBDD
//     snprintf(type_,8,"QBDD");
// #elif defined ZBDD
//     snprintf(type_,8,"ZBDD");
// #elif defined C_FBDD
//     snprintf(type_,8,"C_FBDD");
// #elif defined C_QBDD
//     snprintf(type_,8,"C_QBDD");
// #elif defined CS_FBDD
//     snprintf(type_,8,"CS_FBDD");
// #elif defined CS_QBDD
//     snprintf(type_,8,"CS_QBDD");
// #elif defined ESRBDD
//     snprintf(type_,8,"ESRBDD");
// #elif defined CESRBDD
//     snprintf(type_,8,"CESRBDD");
// #else
//     snprintf(type_,8,"RexBDD");
// #endif

//     snprintf(gz, 64, "/vol/vms/lcdeng/temp_forest_%s.txt.gz", type_);

//     if (fopen(gz, "r") != NULL) {
//         read(&F,ptr5,check_point);
//     }
    for (t=0; t<0x01UL<<(0x01<<levels); t++) {
        temp.edge[0] = ptr4[t / (0x01<<(0x01<<(levels-1)))];
        temp.edge[1] = ptr4[t % (0x01<<(0x01<<(levels-1)))];

        rexdd_reduce_edge(&F, levels, l, temp, &eval);

        ptr5[t] = eval;
        // count number of nodes and write it into file

        for (int i=0; i<0x01<<levels; i++){
            // avoid creating large Function_5
            if (i<0x01<<(levels-1)) {
                eval_real = Function_4[i%(0x01<<(levels-1))][t/(0x01<<(0x01<<(levels-1)))];
            } else {
                eval_real = Function_4[i%(0x01<<(levels-1))][t%(0x01<<(0x01<<(levels-1)))];
            }
            if (rexdd_eval(&F, &eval, levels, Vars_5[i]) == eval_real)
            {
                continue;
            }
            else
            {
                printf("error on function: %lu\n", t);
                f = fopen("error_function.gv", "w+");
                build_gv(f, &F,ptr5[t]);
                fclose(f);

                printf("\n");
                rexdd_check1(0, "Eval error!");
                break;
            }
        }
        // save the forest and root edges every 2^29
        if ((t%(0x01<<((0x01<<levels)-3))==1) || (t == (0x01UL<<(0x01<<levels))-1)){
            save(&F,ptr5,t);
        }
        if (t%(0x01<<((0x01<<levels)-9))==1){
            FILE *pf;
            pf = fopen("progress.txt","w+");
            fprintf(pf,"The progress is %lu / %lu", t, 0x01UL<<(0x01<<levels));
            fclose(pf);
        }
    }
    printf("Done eval!\n");

    export_funsNum(F, levels, ptr5);

    free(ptr5);

    /* ==========================================================================
     *      Summary of the forest
     * ==========================================================================*/
    printf("\n=============================================================\n");
    uint64_t count_nodeLvl[levels+1];
    for (int i=0; i<=levels; i++) {
        count_nodeLvl[i] = 0;
    }
    uint64_t max_number = 0;
    uint_fast64_t p, n;
    for (p=0; p<F.M->pages_size; p++) {
        const rexdd_nodepage_t *page = F.M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_in_use(page->chunk+n)) {
                count_nodeLvl[rexdd_unpack_level(page->chunk+n)]++;
                max_number ++;
            }
        } // for n
    } // for p
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD
    // since they have 2 terminal nodes
    max_number = max_number + 2;
#else 
    max_number = max_number + 1;
#endif

    printf("Total number of nodes in the forest: %llu\n", max_number);

    printf("number of nodes at each level in the forest\n");
    for (int i=1; i<=levels; i++) {
        printf("L%d: %llu\n", i, count_nodeLvl[i]);
    }
    
    /* ==========================================================================
     *      Level 5 test AL AH 
     * ==========================================================================*/
    // printf("\n=============================================================\n");
    // printf("\n======================Testing PushUpAll======================\n");
    // levels = 5;
    // printf("Testing Level Five...\n");
    // bool Vars_5[0x01<<levels][levels+1];
    // bool Function_5[0x01<<levels][0x01<<(0x01<<levels)];
    // make_varsFuns(levels, Vars_4, Function_4, Vars_5, Function_5);

    // // test all 80 cases: swap, complement, rules, target_nodes
    // bool test_function[0x01<<levels];
    // int c_incoming, s_incoming;
    // rexdd_rule_t rules[4];
    // rules[0] = rexdd_rule_AL0;
    // rules[1] = rexdd_rule_AL1;
    // rules[2] = rexdd_rule_AH0;
    // rules[3] = rexdd_rule_AH1;
    // rexdd_node_handle_t l2_target[5];
    // l2_target[0] = ptr2[2].target;
    // l2_target[1] = ptr2[3].target;
    // l2_target[2] = ptr2[4].target;
    // l2_target[3] = ptr2[5].target;
    // l2_target[4] = ptr2[6].target;
    // rexdd_edge_t temp_ALL;
    // int32_t lowNum, highNum;

    // rexdd_unpacked_node_t l2_unpacked_target, l5_unpacked_root;
    // rexdd_edge_t reduced_long, reduced_short;
    // rexdd_edge_label_t l5_incoming_label;
    // l5_incoming_label.complemented = 0;
    // l5_incoming_label.swapped = 0;
    // l5_incoming_label.rule = rexdd_rule_X;
    // l5_unpacked_root.level = levels;

    // // char buffer_err[36];
    // int pass_count = 0, err_count = 0;

    // for (c_incoming=0; c_incoming<2; c_incoming++) {
    //     for (s_incoming=0; s_incoming<2; s_incoming++) {
    //         for (int i=0; i<4; i++) {
    //             for (int j=0; j<5; j++) {
    //                 // initial the incoming edge to level 2 nodes
    //                 temp_ALL.label.complemented = c_incoming;
    //                 temp_ALL.label.swapped = s_incoming;
    //                 temp_ALL.label.rule = rules[i];
    //                 temp_ALL.target = l2_target[j];
    //                 // get its evaluated function and low/high function number
    //                 for (uint16_t k=0; k<0x01<<levels; k++) {
    //                     test_function[k] = rexdd_eval(&F, &temp_ALL, levels, Vars_5[k]);
    //                 }
    //                 lowNum = low_funNum(levels, test_function, Function_4);
    //                 highNum = high_funNum(levels, test_function, Function_4);
    //                 //build unreduced rexdd rooted level 5
    //                 l5_unpacked_root.edge[0] = ptr4[lowNum];
    //                 l5_unpacked_root.edge[1] = ptr4[highNum];

    //                 //reduce long ALL edge
    //                 rexdd_packed_to_unpacked(rexdd_get_packed_for_handle(F.M, temp_ALL.target), &l2_unpacked_target);
    //                 rexdd_reduce_edge(&F,levels,temp_ALL.label,l2_unpacked_target,&reduced_long);
    //                 //check if reduced has the same function
    //                 for (uint16_t ll=0; ll<0x01<<levels; ll++) {
    //                     if (test_function[ll] != rexdd_eval(&F, &reduced_long, levels, Vars_5[ll])){
    //                         printf("/*reduced long edge change the function*/\n");
    //                         break;
    //                     }
    //                 }

    //                 //reduce short edge to root at level 5
    //                 rexdd_reduce_edge(&F,levels, l5_incoming_label, l5_unpacked_root, &reduced_short);
    //                 //check if reduced has the same function
    //                 for (uint16_t ss=0; ss<0x01<<levels; ss++) {
    //                     if (test_function[ss] != rexdd_eval(&F, &reduced_short, levels, Vars_5[ss])){
    //                         printf("/*reduced short edge change the function*/\n");
    //                         break;
    //                     }
    //                 }

    //                 if (!rexdd_edges_are_equal(&reduced_long, &reduced_short)) {
    //                     err_count++;
    //                     printf("->ERROR%d: c: %d; s: %d; rule: %d; target: %d\n",
    //                             err_count, c_incoming, s_incoming, i, j);
    //                     // FILE *out_err;
                        
    //                     // snprintf(buffer_err, 36, "ERR%dL_%d_H_%d_long.gv",err_count, lowNum,highNum);
    //                     // out_err = fopen(buffer_err, "w+");
    //                     // build_gv(out_err,&F,reduced_long);
    //                     // fclose(out_err);

    //                     // snprintf(buffer_err, 36, "ERR%dL_%d_H_%d_short.gv",err_count, lowNum,highNum);
    //                     // out_err = fopen(buffer_err, "w+");
    //                     // build_gv(out_err,&F,reduced_short);
    //                     // fclose(out_err);
    //                 } else {
    //                     pass_count++;
    //                     printf("PASS%d: c: %d; s: %d; rule: %d; target: %d\n",
    //                             pass_count, c_incoming, s_incoming, i, j);
    //                     // FILE *out_eq;
                        
    //                     // snprintf(buffer_err, 36, "PASS%d_L_%d_H_%d_long.gv", pass_count, lowNum,highNum);
    //                     // out_eq = fopen(buffer_err, "w+");
    //                     // build_gv(out_eq,&F,reduced_long);
    //                     // fclose(out_eq);

    //                     // snprintf(buffer_err, 36, "PASS%d_L_%d_H_%d_short.gv", pass_count, lowNum,highNum);
    //                     // out_eq = fopen(buffer_err, "w+");
    //                     // build_gv(out_eq,&F,reduced_short);
    //                     // fclose(out_eq);
                        

    //                     continue;
    //                 }
    //             }

    //         }
    //     }
    // }
    // printf("Done!\n");

    /* ==========================================================================
     *      Uncomment to test check-pointing below
     * ==========================================================================*/
    // uint64_t test_num = 0x01<<(0x01<<4);
    // save(&F, ptr4, test_num);

    // FILE *t;
    // t = fopen("test_save.gv", "w+");
    // build_gv(t, &F, ptr4[56227]);
    // fclose(t);

    // ---------------------------------Free------------------------------------
    rexdd_free_forest(&F);
    // ---------------------------------Free------------------------------------
    
    // rexdd_forest_t F_read;
    // uint64_t ite = 0;
    // F_read.S.num_levels = 5;
    // rexdd_init_forest(&F_read,&(F_read.S));
    // rexdd_edge_t ptr_read[0x01<<(0x01<<4)];
    // read(&F_read,ptr_read, ite);
    // t = fopen("test_read.gv", "w+");
    // build_gv(t, &F_read, ptr_read[56227]);
    // fclose(t);
    // for (int i=0; i<0x01<<(0x01<<4); i++){
    //     if (rexdd_edges_are_equal(&(ptr4[i]), &(ptr_read[i]))) {
    //         continue;
    //     } else {
    //         printf("check_pointing error\n");
    //         break;
    //     }
    // }        
    // printf("check_pointing done\n");

    /* ==========================================================================
     *      Export functions-variables table
     * ==========================================================================*/
#ifdef EXPORT_FUNS
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
#endif

    return 0;
}