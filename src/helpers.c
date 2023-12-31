#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TBD - should this be private, in unpacked.c?
static const char* rexdd_rule_name[] = {
    "EL0",
    "AL0",
    "EL1",
    "AL1",
    "EH0",
    "AH0",
    "EH1",
    "AH1",
    "X",
    "?",
    "?",
//
    "IN",
    "IX",
    "IL0",
    "IL1",
    "IH0",
    "IH1",
    "IEL0",
    "IEL1",
    "IEH0",
    "IEH1",
    "IAL0",
    "IAL1",
    "IAH0",
    "IAH1",
    "I?",
    "I?",
};

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

    fprintf(f, "\t{rank=same v0 T0 [label = \"0\", shape = square]}\n");
    if (F->S.bdd_type == QBDD || F->S.bdd_type == FBDD || F->S.bdd_type == ZBDD 
        || F->S.bdd_type == ESRBDD || F->S.bdd_type == SQBDD || F->S.bdd_type == SFBDD) {
        fprintf(f, "\t{rank=same v0 T1 [label = \"1\", shape = square]}\n");
        }
// #if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD || defined S_QBDD || defined S_FBDD
// #endif

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
    rexdd_node_handle_t max_handle;
    if (F->S.bdd_type == QBDD || F->S.bdd_type == FBDD || F->S.bdd_type == ZBDD
        || F->S.bdd_type == ESRBDD || F->S.bdd_type == SQBDD || F->S.bdd_type == SFBDD){
        max_handle = F->M->pages->first_unalloc;
    } else {
        max_handle = F->M->pages->first_unalloc - 1;
    }
// #if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD || defined S_QBDD || defined S_FBDD
    for (rexdd_node_handle_t t=1; t<=max_handle; t++) {
// #else
//     for (rexdd_node_handle_t t=1; t<F->M->pages->first_unalloc; t++) {
// #endif
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
// #if defined QBDD || defined FBDD || defined ZBDD || defined ESRBDD || defined S_QBDD || defined S_FBDD
    if (F->S.bdd_type == QBDD || F->S.bdd_type == FBDD || F->S.bdd_type == ZBDD || F->S.bdd_type == ESRBDD || F->S.bdd_type == SQBDD || F->S.bdd_type == SFBDD) {
        fprintf(f, "\t{rank=same v0 \"T1\" [label = \"1\", shape = square]}\n");
    }
// #endif

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
    unmark_forest(F);
    mark_nodes(F, handle);
    uint_fast64_t q, n;
    uint64_t num_nodes = 0;
    for (q=0; q<F->M->pages_size; q++) {
        const rexdd_nodepage_t *page = F->M->pages+q;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                num_nodes++;
            }
        } // for n
    } // for p
    unmark_forest(F);
    return num_nodes;
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
        edge.target = rexdd_make_terminal(expect_int(fin));
        // if (skip_whitespace(fin) == '0') {
        //     edge.target = rexdd_make_terminal(0);
        // } else {
        //     edge.target = rexdd_make_terminal(1);
        // }
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
        edge.target = rexdd_make_terminal(expect_int(fin));
        // if (skip_whitespace(fin) == '0') {
        //     edge.target = rexdd_make_terminal(0);
        // } else {
        //     edge.target = rexdd_make_terminal(1);
        // }
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

    fprintf(fout, "type %s\n", F->S.type_name);

    fprintf(fout, "lvls %d\n", F->S.num_levels);
    uint64_t max_handle = 0;
    uint_fast32_t p, n;

    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                max_handle ++;
            }
        } // for n
    } // for p
    fprintf(fout, "maxh %llu\n", max_handle);
    rexdd_unpacked_node_t temp;
    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
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
    snprintf(buffer, 24, "L%d_nodeFun_%s.txt", levels, F.S.type_name);
    snprintf(buffer2, 24, "L%d_funNode_%s.txt", levels, F.S.type_name);

    test = fopen(buffer, "w+");
    test2 = fopen(buffer2, "w+");

    FILE *fount;
    int max_num = 0;
    // int num_term = 0;
    int numFunc[33];
    int num_nodes = 0;
    for (int i=0; i<33; i++) {
        numFunc[i] = 0;
    }
    for (unsigned long i=0; i<0x01UL<<(0x01<<levels); i++) {
        // int terms = countTerm(&F,edges[i].target);
        // if (terms == 0 || terms == 1) {
        //     num_term = 1;
        // } else {
        //     num_term = terms;
        // }
        // int num_nodes = countNodes(&F,edges[i].target) + num_term;
        num_nodes = countNodes(&F,edges[i].target);
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

    for (int i=0; i<33; i++) {
        fprintf(test, "%d %d\n", i, numFunc[i]);
    }

    fclose(test);
    fclose(test2);
    printf("Done counting!\n\n");
    printf("max number of nodes(including terminal) for any level %d %s is: %d\n\n",levels, F.S.type_name, max_num);

}

// unmark all nodes in the forest (initializing for counting the marked nodes)
void unmark_forest(
                rexdd_forest_t *F)
{
    uint_fast64_t p, n;
    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                rexdd_unmark_packed(page->chunk+n);
            }
        } // for n
    } // for p
}

// mark the nonterminal nodes from root in the forest *F. This is used for counting the number of nodes
void mark_nodes(
                rexdd_forest_t *F, 
                rexdd_node_handle_t root)
{
    if (!rexdd_is_terminal(root)) {
        if (!rexdd_is_packed_marked(rexdd_get_packed_for_handle(F->M, root))) {
            rexdd_mark_packed(rexdd_get_packed_for_handle(F->M,root));
            rexdd_node_handle_t low, high;
            low = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root));
            high = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root));
            mark_nodes(F, low);
            mark_nodes(F, high);
        }
    }
    
}

// build a forest recording the input function (2D array of bool with specific container number)
rexdd_edge_t union_minterm(rexdd_forest_t* F, rexdd_edge_t* root, char* minterm, int outcome, uint32_t K)
{
    // the final answer
    rexdd_edge_t ans;
    ans.label.rule = rexdd_rule_X;
    ans.label.complemented = 0;
    ans.label.swapped = 0;

    // terminal case     
    if (K==0) {
        rexdd_node_handle_t termi;
        if ((outcome == 1) || (outcome == 0)) {
            termi = (root->label.complemented) ? rexdd_make_terminal(1-outcome) : rexdd_make_terminal(outcome);
        } else {
            termi = rexdd_make_terminal(outcome);
        }
        rexdd_set_edge(&ans,
                        rexdd_rule_X,
                        0,
                        0,
                        termi);
        return ans;
    }

    /*
     * here means the root edge can not capture the minterm!
     *      we need to break root edge if it's a long edge
     */
    uint32_t root_skip;     // if the root edge is a long edge or not
    if (rexdd_is_terminal(root->target)) {
        root_skip = K;
    } else {
        root_skip = K - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, root->target));
    }

    // determine down pointers
    rexdd_edge_t root_down0, root_down1;
    if (root_skip == 0) {
        // root edge does not skip ndoes at this level
        rexdd_edge_label_t l;
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge((root->label.swapped) ? &root_down1 : &root_down0,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root->target)));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge((root->label.swapped) ? &root_down0 : &root_down1,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root->target)));
        if (root->label.complemented) {
            rexdd_edge_com(&root_down0);
            rexdd_edge_com(&root_down1);
        }
    } else {
        // here means the root edge is a long edge (X, EL, EH, AL, AH)
        if (root->label.rule == rexdd_rule_X) {
            // this is easy
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EL(root->label.rule)) {
            // root edge is EL
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            0,
                            0,
                            rexdd_make_terminal(rexdd_is_one(root->label.rule)));
            rexdd_set_edge(&root_down1,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EH(root->label.rule)) {
            // root edge is EH
            rexdd_set_edge(&root_down0,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            0,
                            0,
                            rexdd_make_terminal(rexdd_is_one(root->label.rule)));
        } else if (rexdd_is_AL(root->label.rule)) {
            // root edge is AL
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AL edge skips >= 2 nodes
                root_down0.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EL1 : rexdd_rule_EL0;
            }
        } else if (rexdd_is_AH(root->label.rule)) {
            // root edge is AH
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AH edge skips >= 2 nodes
                root_down1.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EH1 : rexdd_rule_EH0;
            }
        }
    }

    // Build new node
    rexdd_unpacked_node_t tmp;
    tmp.level = K;

    if (minterm[K] == '1') {
        tmp.edge[1] = union_minterm(F, &root_down1, minterm, outcome, K-1);
        tmp.edge[0] = root_down0;
    } else {
        tmp.edge[0] = union_minterm(F, &root_down0, minterm, outcome, K-1);
        tmp.edge[1] = root_down1;
    }

    rexdd_reduce_edge(F, K, ans.label, tmp, &ans);
    return ans;
}

char*** init_minterms(int num_out, unsigned buf_size, unsigned num_bits)
{
    //
    char*** minterms;
    int x;
    unsigned int y, z;
    minterms = (char***)malloc(num_out * sizeof(char**));
    for (x = 0; x<num_out; x++) {
        minterms[x] = (char**)malloc(buf_size * sizeof(char*));
        for (y = 0; y<buf_size; y++) {
            minterms[x][y] = (char*) malloc((num_bits+2)*sizeof(char));
        }
    }

    for (x = 0; x<num_out; x++) {
        for (y = 0; y<buf_size; y++) {
            for (z = 0; z<num_bits+2; z++) {
                minterms[x][y][z] = 0;
            }
        }
    }
    return minterms;
}

void free_minterms(char*** minterms, int num_out, unsigned buf_size, unsigned num_bits)
{
    //
    if (!minterms) {
        printf("null minterms to free!\n");
        exit(1);
    }
    int x;
    unsigned int y, z;
    for (x = 0; x<num_out; x++) {
        for (y = 0; y<buf_size; y++) {
            for (z = 0; z<num_bits+2; z++) {
                minterms[x][y][z] = 0;
            }
            free(minterms[x][y]);
        }
        free(minterms[x]);
    }
    free(minterms);
}

rexdd_edge_t union_minterms(rexdd_forest_t* F, uint32_t K, rexdd_edge_t* root, char** minterms, int outcome, unsigned int N)
{
    //
    rexdd_edge_t ans;
    if (N == 0) {
        rexdd_set_edge(&ans,
                        root->label.rule,
                        root->label.complemented,
                        root->label.swapped,
                        root->target);
        return ans;
    }

    if (K == 0) {
        rexdd_node_handle_t termi;
        if ((outcome == 1) || (outcome == 0)) {
            termi = (root->label.complemented) ? rexdd_make_terminal(1-outcome) : rexdd_make_terminal(outcome);
        } else {
            termi = rexdd_make_terminal(outcome);
        }
        rexdd_set_edge(&ans,
                        rexdd_rule_X,
                        0,
                        0,
                        termi);
        return ans;
    }

    // Two-finger algorithm to sort 0,1 values in position K
    unsigned left = 0;
    unsigned right = N-1;

    for (;;) {
        // move left to first 1 value
        for ( ; left < right; left++) {
        if ('1' == minterms[left][K]) break;
        }
        // move right to first 0 value
        for ( ; left < right; right--) {
        if ('0' == minterms[right][K]) break;
        }
        // Stop?
        if (left >= right) break;
        // we have a 1 before a 0, swap them
        char* tmp = minterms[left];
        minterms[left] = minterms[right];
        minterms[right] = tmp;
        // For sure we can move them one spot
        ++left;
        --right;
    } // end loop

    if (left < N) {
        if ('0' == minterms[left][K]) left++;
        if (left < N) assert('1' == minterms[left][K]);
        if (left > 0) assert('0' == minterms[left-1][K]);
    }

    uint32_t root_skip;     // if the root edge is a long edge or not
    if (rexdd_is_terminal(root->target)) {
        root_skip = K;
    } else {
        root_skip = K - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, root->target));
    }
    rexdd_edge_t root_down0, root_down1;
    if (root_skip == 0) {
        // root edge does not skip ndoes at this level
        rexdd_edge_label_t l;
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge((root->label.swapped) ? &root_down1 : &root_down0,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root->target)));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge((root->label.swapped) ? &root_down0 : &root_down1,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root->target)));
        if (root->label.complemented) {
            rexdd_edge_com(&root_down0);
            rexdd_edge_com(&root_down1);
        }
    } else {
        // here means the root edge is a long edge (X, EL, EH, AL, AH)
        if (root->label.rule == rexdd_rule_X) {
            // this is easy
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EL(root->label.rule)) {
            // root edge is EL
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            0,
                            0,
                            rexdd_make_terminal(rexdd_is_one(root->label.rule)));
            rexdd_set_edge(&root_down1,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EH(root->label.rule)) {
            // root edge is EH
            rexdd_set_edge(&root_down0,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            0,
                            0,
                            rexdd_make_terminal(rexdd_is_one(root->label.rule)));
        } else if (rexdd_is_AL(root->label.rule)) {
            // root edge is AL
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AL edge skips >= 2 nodes
                root_down0.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EL1 : rexdd_rule_EL0;
            }
        } else if (rexdd_is_AH(root->label.rule)) {
            // root edge is AH
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AH edge skips >= 2 nodes
                root_down1.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EH1 : rexdd_rule_EH0;
            }
        }
    }

    // Build new node
    rexdd_unpacked_node_t tmp;
    tmp.level = K;
    tmp.edge[0] = union_minterms(F, K-1, &root_down0, minterms, outcome, left);
    tmp.edge[1] = union_minterms(F, K-1, &root_down1, minterms + left, outcome, N - left);

    ans.label.rule = rexdd_rule_X;
    ans.label.complemented = 0;
    ans.label.swapped = 0;

    rexdd_reduce_edge(F, K, ans.label, tmp, &ans);
    return ans;
}

rexdd_edge_t rexdd_expand_childEdge(rexdd_forest_t* F, uint32_t r, rexdd_edge_t* e, bool type)
{
    uint32_t skip;
    if (rexdd_is_terminal(e->target)) {
        skip = r;
    } else {
        skip = r - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, e->target));
    }
    rexdd_edge_t ans;
    if (skip == 0) {
        rexdd_edge_label_t l;
        if (e->label.swapped ^ type) {
            rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, e->target), &l);
        } else {
            rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, e->target), &l);
        }
        rexdd_set_edge(&ans,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        (e->label.swapped ^ type)?
                        rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, e->target))
                        :rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, e->target)));
        if (e->label.complemented) rexdd_edge_com(&ans);
        return ans;
    } else {
        if (e->label.rule == rexdd_rule_X) {
            rexdd_set_edge(&ans,
                            e->label.rule,
                            e->label.complemented,
                            e->label.swapped,
                            e->target);
        } else if (rexdd_is_EL(e->label.rule)) {
            if (type) {
                rexdd_set_edge(&ans,
                                (skip == 1) ? rexdd_rule_X : e->label.rule,
                                e->label.complemented,
                                e->label.swapped,
                                e->target);
            } else {
                rexdd_set_edge(&ans,
                                rexdd_rule_X,
                                0,
                                0,
                                rexdd_make_terminal(rexdd_is_one(e->label.rule)));
            }
        } else if (rexdd_is_EH(e->label.rule)) {
            if (type) {
                rexdd_set_edge(&ans,
                                rexdd_rule_X,
                                0,
                                0,
                                rexdd_make_terminal(rexdd_is_one(e->label.rule)));
            } else {
                rexdd_set_edge(&ans,
                                (skip == 1)?rexdd_rule_X:e->label.rule,
                                e->label.complemented,
                                e->label.swapped,
                                e->target);
            }
        } else if (rexdd_is_AL(e->label.rule)) {
            if (type) {
                rexdd_set_edge(&ans,
                                rexdd_rule_X,
                                e->label.complemented,
                                e->label.swapped,
                                e->target);
            } else {
                rexdd_set_edge(&ans,
                                (skip == 1)?rexdd_rule_X:e->label.rule,
                                (skip == 1)?0:e->label.complemented,
                                (skip == 1)?0:e->label.swapped,
                                (skip == 1)?rexdd_make_terminal(rexdd_is_one(e->label.rule)):e->target);
                if (skip == 2) {
                    ans.label.rule = rexdd_is_one(e->label.rule) ? rexdd_rule_EL1 : rexdd_rule_EL0;
                }
            }
        } else {
            // here must be AH
            if (type) {
                rexdd_set_edge(&ans,
                                (skip == 1)?rexdd_rule_X:e->label.rule,
                                (skip == 1)?0:e->label.complemented,
                                (skip == 1)?0:e->label.swapped,
                                (skip == 1)?rexdd_make_terminal(rexdd_is_one(e->label.rule)):e->target);
                if (skip == 2) {
                    ans.label.rule = rexdd_is_one(e->label.rule) ? rexdd_rule_EH1 : rexdd_rule_EH0;
                }
            } else {
                rexdd_set_edge(&ans,
                                rexdd_rule_X,
                                e->label.complemented,
                                e->label.swapped,
                                e->target);
            }
        }
    }
    return ans;
}

void function_2_edge(rexdd_forest_t* F, char* functions, rexdd_edge_t* root_out, int L, unsigned long start, unsigned long end)
{
    rexdd_unpacked_node_t temp;
    temp.level = L;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;
    /*  terminal case */
    if ((end-start == 1) && (L==1)) {
        temp.edge[0].target = rexdd_make_terminal(functions[start]);
        temp.edge[1].target = rexdd_make_terminal(functions[end]);
        
        rexdd_reduce_edge(F, L, l, temp, root_out);
        return;
    }
    const int next_lvl = L-1;
    function_2_edge(F, functions, &temp.edge[0], next_lvl, start, (end-start)/2+start);
    function_2_edge(F, functions, &temp.edge[1], next_lvl, (end-start)/2+start+1, end);
    rexdd_reduce_edge(F, L, l, temp, root_out);
}

rexdd_edge_t build_constant(rexdd_forest_t *F, uint32_t k, int t)
{
    //
    rexdd_edge_t ans;
    rexdd_edge_label_t l;
    l.complemented = 0;
    l.swapped = 0;
    l.rule = rexdd_rule_X;
    rexdd_unpacked_node_t temp;
    if (F->S.bdd_type == QBDD || F->S.bdd_type == CQBDD
        || F->S.bdd_type == SQBDD || F->S.bdd_type == CSQBDD
        || F->S.bdd_type == ZBDD) {
        if (k==0) {
            rexdd_set_edge(&ans, rexdd_rule_X, 0, 0, rexdd_make_terminal(t));
            return ans;
        }
        temp.level = 1;
        rexdd_set_edge(&temp.edge[0],
                        rexdd_rule_X,
                        0,
                        0,
                        rexdd_make_terminal(t));
        rexdd_set_edge(&temp.edge[1],
                        rexdd_rule_X,
                        0,
                        0,
                        rexdd_make_terminal(t));
        rexdd_reduce_edge(F, 1, l, temp, &ans);
        if (k==1) return ans;
        for (uint32_t i=2; i<=k; i++) {
            temp.level = i;
            temp.edge[0] = ans;
            temp.edge[1] = ans;
            rexdd_reduce_edge(F, i, l, temp, &ans);
        }
        return ans;
    } else if (F->S.bdd_type == FBDD || F->S.bdd_type == SFBDD
                || F->S.bdd_type == ESRBDD) {
        // for BDDs that no complement bit
        rexdd_set_edge(&ans, rexdd_rule_X ,0 ,0 , rexdd_make_terminal(t));
        return ans;
    } else {
        // for CF, CSF, CESR, Rex
        rexdd_set_edge(&ans, rexdd_rule_X, (t==1), 0, rexdd_make_terminal(0));
        return ans;
    }
}

rexdd_edge_t build_variable(rexdd_forest_t *F, uint32_t k)
{
    rexdd_edge_t ans;
    rexdd_edge_label_t l;
    l.complemented = 0;
    l.swapped = 0;
    l.rule = rexdd_rule_X;
    rexdd_unpacked_node_t temp;
    temp.level = k;
    temp.edge[0] = build_constant(F, k-1, 0);
    temp.edge[1] = build_constant(F, k-1, 1);
    rexdd_reduce_edge(F, F->S.num_levels, l, temp, &ans);
    if (F->S.bdd_type == QBDD || F->S.bdd_type == CQBDD
        || F->S.bdd_type == SQBDD || F->S.bdd_type == CSQBDD) {
        if (k == F->S.num_levels) return ans;
        for (uint32_t i=k+1; i<=F->S.num_levels; i++) {
            temp.level = i;
            temp.edge[0] = ans;
            temp.edge[1] = ans;
            rexdd_reduce_edge(F, i, l, temp, &ans);
        }
        return ans;
    }
    return ans;
}

long long card_edge(rexdd_forest_t* F, rexdd_edge_t* root, uint32_t lvl)
{
    if (lvl == 0) {
        assert(root->label.rule == rexdd_rule_X);
        return rexdd_terminal_value(root->target) ^ root->label.complemented;
    }
    // determine down pointers
    rexdd_edge_t root_down0, root_down1;
    root_down0 = rexdd_expand_childEdge(F, lvl, root, 0);
    root_down1 = rexdd_expand_childEdge(F, lvl, root, 1);
    long long ans0 = 0, ans1 = 0;
    ans0 = card_edge(F, &root_down0, lvl-1);
    ans1 = card_edge(F, &root_down1, lvl-1);
    return ans0 + ans1;
}

/****************************************************************************
 *  Garbage collection of unmarked nodes in forest F
 *  assuming nodes in use are marked
 */
void gc_unmarked(rexdd_forest_t* F)
{
    /*
     *  Sweep unique table
     */ 
    rexdd_sweep_UT(F->UT);

    /*
     *  Sweep computing table
     */
    rexdd_sweep_CT(F->CT, F->M);

    /*
     *  Ready to sweep all unmarked nodes
     */
    rexdd_sweep_nodeman(F->M);
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
