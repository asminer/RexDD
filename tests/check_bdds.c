#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//================================Helper Function==================================
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
    bool *countNode = malloc((e.target+1)*sizeof(bool));
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
    free(countNode);
}

// Build dot file for BDD with one root edge
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
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD
    fprintf(f, "\t{rank=same v0 \"T1\" [label = \"1\", shape = square]}\n");
#endif

    fprintf(f, "}");
}

// Build dot file for BDDs in the forest with all root edges
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
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD
    for (rexdd_node_handle_t t=1; t<=F->M->pages->first_unalloc; t++) {
#else
    for (rexdd_node_handle_t t=1; t<F->M->pages->first_unalloc; t++) {
#endif
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
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD
    fprintf(f, "\t{rank=same v0 \"T1\" [label = \"1\", shape = square]}\n");
#endif

    fprintf(f, "}");

}

void handleNodes(rexdd_forest_t *F, rexdd_node_handle_t handle, rexdd_node_handle_t *nodes)
{
    if (rexdd_is_terminal(handle)) {
        nodes[0] = 0;
    } else {
        bool flag = 0;
        for (int i=1; i<1<<5;i++) {
            if (nodes[i] == handle){
                flag = 1;
                break;
            } else if (nodes[i] == 0){
                nodes[i] = handle;
                break;
            }
        }
        if (!flag) {
            rexdd_node_handle_t handleL, handleH;
            handleL = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M,handle));
            handleH = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M,handle));

            handleNodes(F, handleL, nodes);
            handleNodes(F, handleH, nodes);
        }

    }
}

int countNodes(rexdd_forest_t *F, rexdd_node_handle_t handle)
{
    rexdd_node_handle_t nodes[1<<5];
    for (int i=0; i<1<<5; i++) {
        nodes[i]=0;
    }
    int count = 0;
    if (rexdd_is_terminal(handle)) {
        count = 0;
    } else {
        handleNodes(F,handle,nodes);
        for (int i=1; i<1<<5; i++) {
            if (nodes[i]!=0) {
                count++;
            }
        }
    }
    return count;
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

static char skip_whitespce(FILE *fin)
{
    for (;;) {
        char c = fgetc(fin);
        if (EOF == c) {
            exit(0);
        }
        if ('\n' == c) continue;
        if (' ' == c) continue;
        if ('\t' == c) continue;
        if ('\r' == c) continue;
        if (':' == c) continue;
        if (',' == c) continue;
        if ('>' == c) continue;

        return c;
    }
}

int expect_int(FILE *fin)
{
    char c = skip_whitespce(fin);
    if ((c > '9') || (c < '0')) exit(0);
    ungetc(c, fin);
    int u;
    fscanf(fin, "%d", &u);
    return u;
}

uint64_t expect_uint64(FILE *fin)
{
    char c = skip_whitespce(fin);
    if ((c > '9') || (c < '0')) exit(0);
    ungetc(c, fin);
    uint64_t u;
    fscanf(fin, "%llu", &u);
    return u;
}

rexdd_rule_t expect_rule(FILE *fin)
{
    char c = skip_whitespce(fin);
    rexdd_rule_t rule = rexdd_rule_X;
    if (c == 'X') {
        rule = rexdd_rule_X;
    } else if (c == 'E') {
        char d = fgetc(fin);
        if (d == 'L') {
            if (fgetc(fin) == '0') {
                rule = rexdd_rule_EL0;
            } else {
                rule = rexdd_rule_EL1;
            }
        } else if (d == 'H') {
            if (fgetc(fin) == '0') {
                rule = rexdd_rule_EH0;
            } else {
                rule = rexdd_rule_EH1;
            }
        }
    } else if (c == 'A') {
        char d = fgetc(fin);
        if (d == 'L') {
            if (fgetc(fin) == '0') {
                rule = rexdd_rule_AL0;
            } else {
                rule = rexdd_rule_AL1;
            }
        } else if (d == 'H') {
            if (fgetc(fin) == '0') {
                rule = rexdd_rule_AH0;
            } else {
                rule = rexdd_rule_AH1;
            }
        }
    }
    return rule;
}

rexdd_edge_t expect_edge(FILE *fin, rexdd_node_handle_t *unique_handles) {
    rexdd_edge_t edge;
    edge.label.rule = expect_rule(fin);
    edge.label.complemented = (skip_whitespce(fin) == '1');
    edge.label.swapped = (skip_whitespce(fin) == '1');
    char c = skip_whitespce(fin);
    if (c == 'T'){
        if (skip_whitespce(fin) == '0') {
            edge.target = rexdd_make_terminal(0);
        } else {
            edge.target = rexdd_make_terminal(1);
        }
    } else {
        ungetc(c,fin);
        fscanf(fin, "%llu", &edge.target);
        edge.target = *(unique_handles + edge.target - 1);
    }
    return edge;
}

void save(rexdd_forest_t *F, rexdd_edge_t edges[], uint64_t t)
{
    FILE *fout;
    // if (fopen("temp_forest.txt", "r") == NULL){
    //     fout = fopen("temp_forest.txt", "w+");
    // } else {
    //     fout = fopen("temp_forest_new.txt", "w+");
    // }
    char gz[64], mv[80], temptxt[64], gz_temp[64], type[8];

#ifdef FBDD
    snprintf(type,8,"FBDD");
#elif defined QBDD
    snprintf(type,8,"QBDD");
#elif defined ZBDD
    snprintf(type,8,"ZBDD");
#elif defined C_FBDD
    snprintf(type,8,"C_FBDD");
#elif defined C_QBDD
    snprintf(type,8,"C_QBDD");
#elif defined CS_FBDD
    snprintf(type,8,"CS_FBDD");
#elif defined CS_QBDD
    snprintf(type,8,"CS_QBDD");
#elif defined ESRBDD
    snprintf(type,8,"ESRBDD");
#elif defined CESRBDD
    snprintf(type,8,"CESRBDD");
#else
    snprintf(type,8,"RexBDD");
#endif
    snprintf(gz, 64, "/vol/vms/lcdeng/temp_forest_%s.txt.gz", type);
    snprintf(mv, 80, "mv -f /vol/vms/lcdeng/temp_forest_%s.txt.gz /vol/vms/lcdeng/backup", type);
    snprintf(temptxt, 64, "/vol/vms/lcdeng/temp_forest_%s.txt", type);
    snprintf(gz_temp, 64, "gzip /vol/vms/lcdeng/temp_forest_%s.txt",type);

    if (fopen(gz, "r") != NULL) {
        fout = popen(mv, "r");
    }
    fout = fopen(temptxt, "w+");

    fprintf(fout, "type ");
    // ifdef TBD for other types
#ifdef FBDD
    fprintf(fout, "FBDD\n");
#elif defined QBDD
    fprintf(fout, "QBDD\n");
#elif defined ZBDD
    fprintf(fout, "ZBDD\n");
#elif defined C_FBDD
    fprintf(fout, "CFBDD\n");
#elif defined C_QDD
    fprintf(fout, "CQBDD\n");
#elif defined CS_FDD
    fprintf(fout, "CSFBDD\n");
#elif defined CS_QDD
    fprintf(fout, "CSQDD\n");
#elif defined ESRBDD
    fprintf(fout, "ESRBDD\n");
#elif defined CESRBDD
    fprintf(fout, "CESRBDD\n");
#else
    fprintf(fout, "REXBDD\n");
#endif
    fprintf(fout, "lvls %d\n", F->S.num_levels);
    uint64_t max_handle = 0;
    uint_fast32_t p, n;

    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_in_use(page->chunk+n)) {
                max_handle ++;
            }
        } // for n
    } // for p
    fprintf(fout, "maxh %llu\n", max_handle);
    rexdd_unpacked_node_t temp;
    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_in_use(page->chunk+n)) {
                rexdd_unpack_low_edge(page->chunk+n, &temp.edge[0].label);
                rexdd_unpack_high_edge(page->chunk+n, &temp.edge[1].label);
                temp.edge[0].target = rexdd_unpack_low_child(page->chunk+n);
                temp.edge[1].target = rexdd_unpack_high_child(page->chunk+n);
                unsigned long long node_id = p*REXDD_PAGE_SIZE+n+1;
                fprintf(fout, "node %llu L %u: 0:<%s,%d,%d", 
                    // handle starts from 1
                    node_id, rexdd_unpack_level(page->chunk+n),
                    rexdd_rule_name[temp.edge[0].label.rule],
                    temp.edge[0].label.complemented,
                    temp.edge[0].label.swapped);
                if (rexdd_is_terminal(temp.edge[0].target)) {
                    fprintf(fout, ",T%llu>,", rexdd_terminal_value(temp.edge[0].target));
                } else {
                    fprintf(fout, ",%llu>,", temp.edge[0].target);
                }
                fprintf(fout, "1:<%s,%d,%d", 
                        rexdd_rule_name[temp.edge[1].label.rule],
                        temp.edge[1].label.complemented,
                        temp.edge[1].label.swapped);
                if (rexdd_is_terminal(temp.edge[1].target)) {
                    fprintf(fout, ",T%llu>\n", rexdd_terminal_value(temp.edge[1].target));
                } else {
                    fprintf(fout, ",%llu>\n", temp.edge[1].target);
                }
            }
        } // for n
    } // for p
    
    // wirte number of iterations that checked eval so far
    fprintf(fout, "root %llu:\n", t);
    for (uint64_t i=0; i<t; i++) {
        fprintf(fout, "<%s,%d,%d",
                rexdd_rule_name[edges[i].label.rule],
                edges[i].label.complemented,
                edges[i].label.swapped);
        if (rexdd_is_terminal(edges[i].target)) {
            fprintf(fout, ",T%llu>\n", rexdd_terminal_value(edges[i].target));
        } else {
            fprintf(fout, ",%llu>\n", edges[i].target);
        }
    }
    
    fclose(fout);

    // if (fopen("temp_forest_new.txt", "r") != NULL) {
    //     remove("temp_forest.txt");
    //     rename("temp_forest_new.txt", "temp_forest.txt");
    // }
    fout = popen(gz_temp, "r");
    pclose(fout);
}

void read(rexdd_forest_t *F, rexdd_edge_t edges[], uint64_t t)
{
    FILE *fin;
    fin = fopen("temp_forest.txt", "r");

    char type[10];
    int level;
    uint64_t handles;
    // get the type, levels and max number of nodes to decide something
    fscanf (fin, " type %s lvls %d maxh %llu", type, &level, &handles);
    F->S.num_levels = level;
    fclose(fin);

    // array to store unique handle 
    rexdd_node_handle_t *unique_handles = malloc(handles*sizeof(rexdd_node_handle_t));
    uint64_t loc = 0;

    fin = fopen("temp_forest.txt", "r");
    
    // temp unpacked node, root edge
    rexdd_unpacked_node_t node;
    while (!feof(fin))
    {
        char buffer[4];
        buffer[0] = skip_whitespce(fin);
        for (int i=1; i<4; i++) {
            buffer[i] = skip_whitespce(fin);
        }
        if (buffer[0] == 'n' && buffer[1] == 'o' && buffer[2] == 'd' && buffer[3] == 'e'){
            // build the node at location
            loc = expect_uint64(fin) - 1;
            char c = skip_whitespce(fin);
            if (c == 'L') {
                // set node's level
                node.level = expect_int(fin);
                // set low edge
                if (skip_whitespce(fin) == '0' && skip_whitespce(fin) == '<') {
                    // set node's edges
                    node.edge[0] = expect_edge(fin, unique_handles);
                    if (skip_whitespce(fin) == '1' && skip_whitespce(fin) == '<') {
                        node.edge[1] = expect_edge(fin, unique_handles);
                    }
                }
            }
            // insert this node into unique table and put its handle in the array
            *(unique_handles + loc) = rexdd_insert_UT(F->UT, rexdd_nodeman_get_handle(F->M,&node));

        } else if (buffer[0] == 'r' && buffer[1] == 'o' && buffer[2] == 'o' && buffer[3] == 't') {
            t = expect_uint64(fin);
            printf ("number of roots: %llu\n", t);
            for (uint64_t i=0; i<t; i++) {
                if (skip_whitespce(fin) == '<') {
                    edges[i] = expect_edge(fin, unique_handles);
                }
                // printf ("root: %d, %d, %d, %llu\n", root.label.rule, root.label.complemented, root.label.swapped, root.target);
            }
            break;
        }
    }
    free(unique_handles);

    fclose(fin);
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

void export_funsNum(rexdd_forest_t F, int levels, rexdd_edge_t edges[])
{
    printf("Counting number of nodes...\n");
    FILE *test, *test2;
    char buffer[24], buffer2[24];
#ifdef QBDD
    snprintf(buffer, 24, "L%d_nodeFun_QBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_QBDD.txt", levels);
#endif
#ifdef C_QBDD
    snprintf(buffer, 24, "L%d_nodeFun_CQBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_CQBDD.txt", levels);
#endif
#ifdef CS_QBDD
    snprintf(buffer, 24, "L%d_nodeFun_CSQBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_CSQBDD.txt", levels);
#endif

#ifdef FBDD
    snprintf(buffer, 24, "L%d_nodeFun_FBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_FBDD.txt", levels);
#endif
#ifdef C_FBDD
    snprintf(buffer, 24, "L%d_nodeFun_CFBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_CFBDD.txt", levels);
#endif
#ifdef CS_FBDD
    snprintf(buffer, 24, "L%d_nodeFun_CSFBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_CSFBDD.txt", levels);
#endif

#ifdef ZBDD
    snprintf(buffer, 24, "L%d_nodeFun_ZBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_ZBDD.txt", levels);
#endif

#ifdef ESRBDD
    snprintf(buffer, 24, "L%d_nodeFun_ESRBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_ESRBDD.txt", levels);
#endif
#ifdef CESRBDD
    snprintf(buffer, 24, "L%d_nodeFun_CESRBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_CESRBDD.txt", levels);
#endif

#ifdef REXBDD
    snprintf(buffer, 24, "L%d_nodeFun_RexBDD.txt", levels);
    snprintf(buffer2, 24, "L%d_funNode_RexBDD.txt", levels);
#endif
    test = fopen(buffer, "w+");
    test2 = fopen(buffer2, "w+");

    int max_num = 0;
    int num_term = 0;
    for (unsigned long i=0; i<0x01UL<<(0x01<<levels); i++) {
        if (countTerm(&F,edges[i].target) == 0 || countTerm(&F,edges[i].target) == 1) {
            num_term = 1;
        } else {
            num_term = countTerm(&F,edges[i].target);
        }
        int num_nodes = countNodes(&F, edges[i].target) + num_term;
        if (max_num < num_nodes) {
            max_num = num_nodes;
        }
        // fprintf(test2, "%lu %d\n", i, num_nodes);
    }
#ifdef QBDD
    printf("max number of nodes(including terminal) for any level %d QBDD is: %d\n\n",levels, max_num);
#endif
#ifdef C_QBDD
    printf("max number of nodes(including terminal) for any level %d CQBDD is: %d\n\n",levels, max_num);
#endif
#ifdef CS_QBDD
    printf("max number of nodes(including terminal) for any level %d CSQBDD is: %d\n\n",levels, max_num);
#endif

#ifdef FBDD
    printf("max number of nodes(including terminal) for any level %d FBDD is: %d\n\n",levels, max_num);
#endif
#ifdef C_FBDD
    printf("max number of nodes(including terminal) for any level %d CFBDD is: %d\n\n",levels, max_num);
#endif
#ifdef CS_FBDD
    printf("max number of nodes(including terminal) for any level %d CSFBDD is: %d\n\n",levels, max_num);
#endif

#ifdef ZBDD
    printf("max number of nodes(including terminal) for any level %d ZBDD is: %d\n\n",levels, max_num);
#endif

#ifdef ESRBDD
    printf("max number of nodes(including terminal) for any level %d ESRBDD is: %d\n\n",levels, max_num);
#endif
#ifdef CESRBDD
    printf("max number of nodes(including terminal) for any level %d CESRBDD is: %d\n\n",levels, max_num);
#endif

#ifdef REXBDD
    printf("max number of nodes(including terminal) for any level %d RexBDD is: %d\n\n",levels, max_num);
#endif
    int numFunc[max_num+1];
    for (int i=0; i<max_num+1; i++) {
        numFunc[i] = 0;
    }
    FILE *fount;

    for (unsigned long i=0; i<0x01UL<<(0x01<<levels); i++) {
        if (countTerm(&F,edges[i].target) == 0 || countTerm(&F,edges[i].target) == 1) {
            num_term = 1;
        } else {
            num_term = countTerm(&F,edges[i].target);
        }
        numFunc[countNodes(&F,edges[i].target) + num_term]++;
        if (i%(0x01UL<<((0x01<<levels)-9))==1) {
            fount = fopen("count_nodes_progress.txt","w+");
            fprintf(fount, "Count number progress is %lu / %lu",i,0x01UL<<(0x01<<levels));
        }
    }
    fprintf(fount, "\nDone level %d", levels);
    fclose(fount);

    for (int i=1; i<max_num+1; i++) {
        fprintf(test, "%d %d\n", i, numFunc[i]);
    }
    fclose(test);
    fclose(test2);
    printf("Done counting!\n\n");
}



//=================================================================================
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
    fclose(f);

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
        if (t%(0x01<<((0x01<<levels)-3))==1){
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
    // build_gv(t, &F, ptr4[4982]);
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
    // build_gv(t, &F_read, ptr_read[4982]);
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