#include "forest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int countDistinct(rexdd_edge_t buffer[], int size, rexdd_edge_t ptr[])
{
    int i, j, count = 1;
    ptr[0] = buffer[0];
    for (i=1; i<size; i++) {
        for (j=0; j<i; j++) {
            if (rexdd_edges_are_equal( &buffer[i], &buffer[j])){
                break;
            }
        }
        if (i==j) {
            ptr[count] = buffer[i];
            count++;
        }
    }
    return count;
}

int main()
{
    printf("Initializing...\n");
    rexdd_forest_t F;
    rexdd_forest_settings_t s;

    rexdd_default_forest_settings(5, &s);
    rexdd_init_forest(&F, &s);

    int levels;

    printf("Testing Level One...\n");
    rexdd_unpacked_node_t Temp, term;
    rexdd_edge_label_t Incoming;
    int s_incoming, c_incoming, s_0, c_0, s_1, c_1;

    term.level=0;
    term.edge[0].target=0;
    term.edge[1].target=0;
    term.edge[0].label.rule=rexdd_rule_X;
    term.edge[1].label.rule=rexdd_rule_X;
    term.edge[0].label.complemented=0;
    term.edge[1].label.complemented=0;
    term.edge[0].label.swapped=0;
    term.edge[1].label.swapped=0;
    rexdd_edge_t buffer[64+18];
    int n_edge=0;

    levels = 1;
    rexdd_edge_t ptr1[0x01 << (0x01 << levels)];

    Temp.level = levels;
    for (c_incoming=0; c_incoming<2; c_incoming++) {
        for (s_incoming=0; s_incoming<2; s_incoming++) {
            Incoming.rule = rexdd_rule_X;
            Incoming.swapped = s_incoming;
            Incoming.complemented = c_incoming;
            rexdd_edge_label_t l = Incoming;
            for (s_0=0; s_0<2; s_0++) {
                for (c_0=0; c_0<2; c_0++) {
                    for (s_1=0; s_1<2; s_1++) {
                        for (c_1=0; c_1<2; c_1++) {
                            // Build unpacked node Temp
                            Temp.edge[0].target = rexdd_make_terminal(0);
                            Temp.edge[1].target = rexdd_make_terminal(0);
                            Temp.edge[0].label.rule = rexdd_rule_X;
                            Temp.edge[1].label.rule = rexdd_rule_X;
                            Temp.edge[0].label.complemented = c_0;
                            Temp.edge[1].label.complemented = c_1;
                            Temp.edge[0].label.swapped = s_0;
                            Temp.edge[1].label.swapped = s_1;

                            rexdd_edge_t reduced;
                            rexdd_reduce_edge(&F, levels, l, Temp, &reduced);

                            buffer[n_edge] = reduced;
                            n_edge++;
                        }
                    }                    
                }
            }

        }
        for (rexdd_rule_t r=0; r<9; r++){
            Incoming.rule = r;
            Incoming.swapped = 0;
            Incoming.complemented = c_incoming;
            rexdd_edge_label_t l = Incoming;

            rexdd_edge_t reduced;
            rexdd_reduce_edge(&F, levels, l, term, &reduced);
            buffer[n_edge] = reduced;
            n_edge++;
        }
    }

    if (countDistinct(buffer, 64+18, ptr1) != 0x01 << (0x01 << levels)) {
        printf("Level one check failed\n");
    } else {
        printf("level one reduced edge: %d\n", 0x01 << (0x01 << levels));
        char snp[20];
        for (int i=0; i<0x01 << (0x01 << levels); i++){
            rexdd_snprint_edge(snp, 20, ptr1[i]);
            printf("%s\n", snp);
        }
        printf("\nDone!\n");
    }

    /*=================================================================================*/
    printf("Testing Level Two...\n");
    levels = 2;
    n_edge = 0;
    rexdd_edge_t ptr2[0x01 << (0x01 << levels)];
    rexdd_edge_t buffer2[64+18];

    Temp.level = levels;

    for (c_incoming=0; c_incoming<2; c_incoming++) {
        for (s_incoming=0; s_incoming<2; s_incoming++) {
            Incoming.rule = rexdd_rule_X;
            Incoming.complemented = c_incoming;
            Incoming.swapped = s_incoming;
            rexdd_edge_label_t l = Incoming;
            for (int i=0; i<4; i++) {
                for (int j = 0; j < 4; j++) {
                    Temp.edge[0] = ptr1[i];
                    Temp.edge[1] = ptr1[j];

                    rexdd_edge_t reduced;
                    rexdd_reduce_edge(&F, levels, l, Temp, &reduced);

                    buffer2[n_edge] = reduced;
                    n_edge++;
                }
            }
        }
        for (rexdd_rule_t r=0; r<9; r++){
            Incoming.rule = r;
            Incoming.swapped = 0;
            Incoming.complemented = c_incoming;
            rexdd_edge_label_t l = Incoming;

            rexdd_edge_t reduced;
            rexdd_reduce_edge(&F, levels, l, term, &reduced);
            buffer2[n_edge] = reduced;
            n_edge++;
        }
    }

    if (countDistinct(buffer2, 64+18, ptr2) != 0x01 << (0x01 << levels)) {
        printf("Level one check failed\n");
    } else {
        printf("level one reduced edge: %d\n", 0x01 << (0x01 << levels));
        char snp[20];
        for (int i=0; i<0x01 << (0x01 << levels); i++){
            rexdd_snprint_edge(snp, 20, ptr2[i]);
            printf("%s\n", snp);
        }
        printf("\nDone!\n");
    }
    
    /*=================================================================================*/
    printf("Testing Level Three...\n");
    levels = 3;
    n_edge = 0;
    rexdd_edge_t ptr3[0x01 << (0x01 << levels)];
    rexdd_edge_t buffer3[2*2*16*16+18];

    Temp.level = levels;

    for (c_incoming=0; c_incoming<2; c_incoming++) {
        for (s_incoming=0; s_incoming<2; s_incoming++) {
            Incoming.rule = rexdd_rule_X;
            Incoming.complemented = c_incoming;
            Incoming.swapped = s_incoming;
            rexdd_edge_label_t l = Incoming;
            for (int i=0; i<16; i++) {
                for (int j = 0; j < 16; j++) {
                    Temp.edge[0] = ptr2[i];
                    Temp.edge[1] = ptr2[j];

                    rexdd_edge_t reduced;
                    rexdd_reduce_edge(&F, levels, l, Temp, &reduced);

                    buffer3[n_edge] = reduced;
                    n_edge++;
                }
            }
        }
        for (rexdd_rule_t r=0; r<9; r++){
            Incoming.rule = r;
            Incoming.swapped = 0;
            Incoming.complemented = c_incoming;
            rexdd_edge_label_t l = Incoming;

            rexdd_edge_t reduced;
            rexdd_reduce_edge(&F, levels, l, term, &reduced);
            buffer3[n_edge] = reduced;
            n_edge++;
        }
    }

    if (countDistinct(buffer3, 2*2*16*16+18, ptr3) != 0x01 << (0x01 << levels)) {
        printf("Level one check failed\n");
    } else {
        printf("level one reduced edge: %d\n", 0x01 << (0x01 << levels));
        char snp[20];
        for (int i=0; i<0x01 << (0x01 << levels); i++){
            rexdd_snprint_edge(snp, 20, ptr3[i]);
            printf("%s\n", snp);
        }
        printf("\nDone!\n");
    }

/*=================================================================================*/
    printf("Testing Level Four...\n");
    levels = 4;
    n_edge = 0;
    rexdd_edge_t ptr4[0x01 << (0x01 << levels)];
    rexdd_edge_t buffer4[2*2*256*256+18];

    Temp.level = levels;

    for (c_incoming=0; c_incoming<2; c_incoming++) {
        for (s_incoming=0; s_incoming<2; s_incoming++) {
            Incoming.rule = rexdd_rule_X;
            Incoming.complemented = c_incoming;
            Incoming.swapped = s_incoming;
            rexdd_edge_label_t l = Incoming;
            for (int i=0; i<256; i++) {
                for (int j = 0; j < 256; j++) {
                    Temp.edge[0] = ptr3[i];
                    Temp.edge[1] = ptr3[j];

                    rexdd_edge_t reduced;
                    rexdd_reduce_edge(&F, levels, l, Temp, &reduced);

                    buffer4[n_edge] = reduced;
                    n_edge++;
                }
            }
        }
        for (rexdd_rule_t r=0; r<9; r++){
            Incoming.rule = r;
            Incoming.swapped = 0;
            Incoming.complemented = c_incoming;
            rexdd_edge_label_t l = Incoming;

            rexdd_edge_t reduced;
            rexdd_reduce_edge(&F, levels, l, term, &reduced);
            buffer4[n_edge] = reduced;
            n_edge++;
        }
    }

    if (countDistinct(buffer4, 2*2*256*256+18, ptr4) != 0x01 << (0x01 << levels)) {
        printf("Level one check failed\n");
    } else {
        printf("level one reduced edge: %d\n", 0x01 << (0x01 << levels));
        char snp[20];
        for (int i=0; i<0x01 << (0x01 << levels); i++){
            rexdd_snprint_edge(snp, 20, ptr4[i]);
            printf("%s\n", snp);
        }
        printf("\nDone!\n");
    }


    rexdd_free_forest(&F);

    return 0;
}