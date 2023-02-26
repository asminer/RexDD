#include "helpers.h"

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
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD || defined S_QBDD || defined S_FBDD
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
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD || defined S_QBDD || defined S_FBDD
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
#if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD || defined S_QBDD || defined S_FBDD
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
        for (int i=1; i<1<<(F->S.num_levels);i++) {
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
    for (int i=0; i<1<<(F->S.num_levels); i++) {
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

char skip_whitespace(FILE *fin)
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
    char c = skip_whitespace(fin);
    if ((c > '9') || (c < '0')) exit(0);
    ungetc(c, fin);
    int u;
    fscanf(fin, "%d", &u);
    return u;
}

uint64_t expect_uint64(FILE *fin)
{
    char c = skip_whitespace(fin);
    if ((c > '9') || (c < '0')) exit(0);
    ungetc(c, fin);
    uint64_t u;
    fscanf(fin, "%llu", &u);
    return u;
}

rexdd_rule_t expect_rule(FILE *fin)
{
    char c = skip_whitespace(fin);
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

rexdd_edge_t expect_edge(FILE *fin, rexdd_node_handle_t *unique_handles)
{
    rexdd_edge_t edge;
    edge.label.rule = expect_rule(fin);
    edge.label.complemented = (skip_whitespace(fin) == '1');
    edge.label.swapped = (skip_whitespace(fin) == '1');
    char c = skip_whitespace(fin);
    if (c == 'T'){
        if (skip_whitespace(fin) == '0') {
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

rexdd_edge_t expect_reduced_edge(FILE *fin, rexdd_edge_t *reduced_edges)
{
    rexdd_edge_t edge;
    edge.label.rule = expect_rule(fin);
    edge.label.complemented = (skip_whitespace(fin) == '1');
    edge.label.swapped = (skip_whitespace(fin) == '1');
    char c = skip_whitespace(fin);
    if (c == 'T'){
        if (skip_whitespace(fin) == '0') {
            edge.target = rexdd_make_terminal(0);
        } else {
            edge.target = rexdd_make_terminal(1);
        }
    } else {
        ungetc(c,fin);
        fscanf(fin, "%llu", &edge.target);
        edge = *(reduced_edges + edge.target - 1);
    }
    return edge;
}

/* save forest *F and root edges *edges into file *fout, fclose not included
   After this, mv and gzip may be needed to manage the storage space */
void save(FILE *fout, rexdd_forest_t *F, rexdd_edge_t edges[], uint64_t t)
{

    // char gz[64], mv[80], temptxt[64], gz_temp[64];

    // snprintf(gz, 64, "/vol/vms/lcdeng/temp_forest_%s.txt.gz", TYPE);
    // snprintf(mv, 80, "mv -f /vol/vms/lcdeng/temp_forest_%s.txt.gz /vol/vms/lcdeng/backup", TYPE);
    // snprintf(temptxt, 64, "/vol/vms/lcdeng/temp_forest_%s.txt", TYPE);
    // snprintf(gz_temp, 64, "gzip /vol/vms/lcdeng/temp_forest_%s.txt",TYPE);

    // if (fopen(gz, "r") != NULL) {
    //     fout = popen(mv, "r");
    // }
    // fout = fopen(temptxt, "w+");
    // // fout = fopen(type, "w+");

    fprintf(fout, "type %s\n", TYPE);

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
}

/*  Read forest and root edges from file *fin into *F and *edges*/
void read_bdd(FILE *fin, rexdd_forest_t *F, rexdd_edge_t edges[], uint64_t t)
{
    // FILE *fin;

    // char gz[64], gunzip[80], temptxt[64], gz_temp[64];

    // snprintf(gz, 64, "/vol/vms/lcdeng/temp_forest_%s.txt.gz", TYPE);
    // snprintf(gunzip, 80, "gunzip /vol/vms/lcdeng/temp_forest_%s.txt.gz", TYPE);
    // snprintf(temptxt, 64, "/vol/vms/lcdeng/temp_forest_%s.txt", TYPE);
    // snprintf(gz_temp, 64, "gzip /vol/vms/lcdeng/temp_forest_%s.txt",TYPE);
    // if (fopen(gz, "r") != NULL) {
    //     fin = popen(gunzip, "r");
    // }
    // fin = fopen(temptxt, "r");
    // fin = fopen(type_, "r");

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

    // temp unpacked node, root edge
    rexdd_unpacked_node_t node;
    while (!feof(fin))
    {
        char buffer[4];
        buffer[0] = skip_whitespace(fin);
        for (int i=1; i<4; i++) {
            buffer[i] = skip_whitespace(fin);
        }
        if (buffer[0] == 'n' && buffer[1] == 'o' && buffer[2] == 'd' && buffer[3] == 'e'){
            // build the node at location
            loc = expect_uint64(fin) - 1;
            char c = skip_whitespace(fin);
            if (c == 'L') {
                // set node's level
                node.level = expect_int(fin);
                // set low edge
                if (skip_whitespace(fin) == '0' && skip_whitespace(fin) == '<') {
                    // set node's edges
                    node.edge[0] = expect_edge(fin, unique_handles);
                    if (skip_whitespace(fin) == '1' && skip_whitespace(fin) == '<') {
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
                if (skip_whitespace(fin) == '<') {
                    edges[i] = expect_edge(fin, unique_handles);
                }
                // printf ("root: %d, %d, %d, %llu\n", root.label.rule, root.label.complemented, root.label.swapped, root.target);
            }
            break;
        }
    }
    free(unique_handles);

    // fclose(fin);
    // fin = popen(gz_temp, "r");
    // pclose(fin);
}

void export_funsNum(rexdd_forest_t F, int levels, rexdd_edge_t edges[])
{
    printf("Counting number of nodes...\n");
    FILE *test, *test2;
    char buffer[24], buffer2[24];
    snprintf(buffer, 24, "L%d_nodeFun_%s.txt", levels, TYPE);
    snprintf(buffer2, 24, "L%d_funNode_%s.txt", levels, TYPE);

    test = fopen(buffer, "w+");
    test2 = fopen(buffer2, "w+");

    FILE *fount;
    int max_num = 0;
    int num_term = 0;
    int numFunc[33];
    for (int i=0; i<33; i++) {
        numFunc[i] = 0;
    }
    for (unsigned long i=0; i<0x01UL<<(0x01<<levels); i++) {
        int terms = countTerm(&F,edges[i].target);
        if (terms == 0 || terms == 1) {
            num_term = 1;
        } else {
            num_term = terms;
        }
        int num_nodes = countNodes(&F,edges[i].target) + num_term;
        numFunc[num_nodes]++;
        if (max_num < num_nodes) {
            max_num = num_nodes;
        }
        if (i%(0x01UL<<((0x01<<levels)-9))==1) {
            fount = fopen("count_nodes_progress.txt","w+");
            fprintf(fount, "Count number progress is %lu / %lu",i,0x01UL<<(0x01<<levels));
            fclose(fount);
        }
        fprintf(test2, "%lu %d\n", i, num_nodes);
    }
    fount = fopen("count_nodes_progress.txt","w+");
    fprintf(fount, "\nDone level %d", levels);
    fclose(fount);

    for (int i=1; i<33; i++) {
        fprintf(test, "%d %d\n", i, numFunc[i]);
    }

    fclose(test);
    fclose(test2);
    printf("Done counting!\n\n");
    printf("max number of nodes(including terminal) for any level %d %s is: %d\n\n",levels, TYPE, max_num);

}










// /****************************************************************************
//  * Helper: write an edge in dot format.
//  * The source node, and the ->, should have been written already.
//  *
//  *  @param  out     File stream to write to
//  *  @param  e       edge
//  *  @param  solid   if true, write a solid edge; otherwise dashed.
//  */
// static void write_dot_edge(FILE* out, rexdd_edge_t e, bool solid)
// {
//     char label[128];
//     unsigned j, commas;

//     if (rexdd_is_terminal(e.target)) {
//         fprintf(out, "T%d", rexdd_terminal_value(e.target));
//     } else {
//         fprintf(out, "N%x_%x",
//             rexdd_nonterminal_handle(e.target) / REXDD_PAGE_SIZE,
//             rexdd_nonterminal_handle(e.target) % REXDD_PAGE_SIZE
//         );
//     }
//     if (solid) {
//         fprintf(out, " [style=solid,  ");
//     } else {
//         fprintf(out, " [style=dashed, ");
//     }

//     commas = 0;
//     rexdd_snprint_edge(label, 128, e);
//     fprintf(out, "label=\"");
//     for (j=0; j<128; j++) {
//         if (0 == label[j]) break;
//         if (',' != label[j]) {
//             fputc(label[j], out);
//             continue;
//         }
//         ++commas;
//         if (commas < 2) {
//             fputc(',', out);
//             continue;
//         }
//         fputc('>', out);
//         break;
//     }
//     fprintf(out, "\"];\n");
// }

// /****************************************************************************
//  *
//  *  Create a dot file for the forest,
//  *  with the given root edge.
//  *      TBD: use forest roots instead
//  *      TBD: allow names to be attached to functions
//  *      TBD: only display root edges with names?
//  *      TBD: only display nodes reachable from a root edge?
//  *
//  *      @param  out         Output stream to write to
//  *      @param  F           Forest to use.
//  *
//  *      @param  e           Root edge, for now.
//  *
//  */
// void rexdd_export_dot(FILE* out, const rexdd_forest_t *F, rexdd_edge_t e)
// {
//     unsigned i;
//     rexdd_check1(F, "Null forest in rexdd_export_dot");

//     fprintf(out, "//\n// Automatically generated by rexdd_export_dot\n//\n");
//     fprintf(out, "digraph g {\n");
//     fprintf(out, "    rankdir=TB;\n\n");

//     fprintf(out, "// Levels.\n\n");

//     fprintf(out, "    node [shape=none];\n");
//     fprintf(out, "    v0[label=\"\"];\n");
//     for (i=0; i<= F->S.num_levels+1; i++) {
//         fprintf(out, "    v%u[label=\"x%u\"];\n", i, i);
//     }
//     fprintf(out, "    v%u[label=\"\"];\n\n", F->S.num_levels+1);

//     for (i=0; i<= F->S.num_levels; i++) {
//         fprintf(out, "    v%u -> v%u [style=invis];\n", i+1, i);
//     }

//     fprintf(out, "\n// Terminal nodes\n\n");

//     fprintf(out, "    { rank=same; v0 T0[shape=rect, label=\"0\"]; }\n");

//     fprintf(out, "\n// Nonerminal nodes\n\n");

//     fprintf(out, "    node [shape=circle];\n");

//     //
//     // Declare all nodes before writing outgoing edges
//     //
//     uint_fast32_t p, n;
//     for (p=0; p<F->M->pages_size; p++) {
//         const rexdd_nodepage_t *page = F->M->pages+p;
//         for (n=0; n<page->first_unalloc; n++) {
//             if (rexdd_is_packed_in_use(page->chunk+n)) {
//                 fprintf(out, "    { rank=same; v%u N%x_%x[label=\"N%x_%x\"]; }\n",
//                         rexdd_unpack_level(page->chunk+n), p, n, p, n);
//             }
//         } // for n
//     } // for p
//     fprintf(out, "\n");

//     //
//     // Traverse again, to write edges
//     //
//     rexdd_unpacked_node_t node;
//     char label[128];
//     unsigned commas, j;

//     for (p=0; p<F->M->pages_size; p++) {
//         const rexdd_nodepage_t *page = F->M->pages+p;
//         for (n=0; n<page->first_unalloc; n++) {
//             if (rexdd_is_packed_in_use(page->chunk+n)) {
//                 rexdd_packed_to_unpacked(page->chunk+n, &node);

//                 for (i=0; i<2; i++) {
//                     fprintf(out, "    N%x_%x -> ", p, n);
//                     write_dot_edge(out, node.edge[i], i);
//                 } // for i


//             } // node is in use
//         } // for n
//     } // for p
//     fprintf(out, "\n");

//     // Root edge

//     fprintf(out, "\n// Root edges\n\n");

//     fprintf(out, "    { rank=same; v%u root[shape=none, label=\"r\"]; }\n",
//         F->S.num_levels+1
//     );
//     fprintf(out, "    r -> ");
//     write_dot_edge(out, e, true);
//     fprintf(out, "}\n");
// }
