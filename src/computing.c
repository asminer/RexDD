
#include "computing.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 *  Null-terminated sequence of primes to use as hash table sizes.
 *  About this list:
 *      (1) They approximately double in size until about 2 million
 *      (2) They increase by about 1.4 (square root of 2) until
 *          about 400 million
 *      (3) They increase by about 1.26 (cube root of 3) to the end
 *      (4) The list stops around 2^49.
 *
 */
static const uint_fast64_t primes[] = {
    1009, 2027, 4057, 8117, 16249, 32503, 65011, 130027, 260081, 520193,
    1040387, 2080777, 2913139, 4078397, 5709757, 7993663, 11191153, 15667633,
    21934691, 30708577, 42992021, 60188837, 84264377, 117970133, 165158207,
    231221491, 323710097, 407874751, 513922187, 647541971, 815902909,
    1028037671, 1295327513, 1632112667, 2056461989, 2591142131, 3264839087,
    4113697319, 5183258669, 6530905951, 8228941499, 10368466351, 13064267633,
    16460977223, 20740831313, 26133447503, 32928143879, 41489461309,
    52276721261, 65868668837, 82994522759, 104573098691, 131762104351,
    166020251489, 209185516877, 263573751283, 332102926681, 418449687637,
    527246606423, 664330724111, 837056712409, 1054691457653, 1328911236643,
    1674428158229, 2109779479439, 2658322144133, 3349485901661, 4220352236131,
    5317643817529, 6700231210093, 8442291324737, 10637287069183,
    13402981707197, 16887756951073, 21278573758373, 26811002935583,
    33781863698897, 42565148260643, 53632086808429, 67576429378651,
    85146301017101, 107284339281571, 135178267494863, 170324617043543,
    214609017474941, 270407362018439, 340713276143243, 429298727940493,
    540916397205031, 0 };

/****************************************************************************
 *
 *  Initialize a computing table
 *
 */
void rexdd_init_CT(rexdd_comp_table_t *CT)
{
    rexdd_sanity1(CT, "Null unique table");
    CT->size_index = 0;
    CT->num_entries = 0;
    CT->table = malloc(primes[0]*sizeof(rexdd_edge_in_ct));
    if (!CT->table) {
        printf("Fail malloc table to initialize\n");
        exit(1);
    }
    for (uint_fast64_t i=0; i<primes[0]; i++) {
        CT->table[i].lvl = 0;
        CT->table[i].edge1 = 0;
        CT->table[i].edge2 = 0;
        CT->table[i].edgeA = 0;
    }
    // initialized to 0, but not malloc
}

/****************************************************************************
 *
 *  Free memory for a computing table.
 */
void rexdd_free_CT(rexdd_comp_table_t *CT)
{
    rexdd_sanity1(CT, "Null unique table");
    for (uint_fast64_t i=0; i<primes[CT->size_index]; i++) {
        CT->table[i].lvl = 0;
        if (CT->table[i].edge1) {
            free(CT->table[i].edge1);
        }
        if (CT->table[i].edge2) {
            free(CT->table[i].edge2);
        }
        if (CT->table[i].edgeA) {
            free(CT->table[i].edgeA);
        }
    }
    CT->num_entries = 0;
    CT->size_index = 0;
    CT->table = 0;
}


/****************************************************************************
 *
 *  Check entry in a computing table for AND operation.
 *  If cached, returns 1 and corresponding edge is set to *e;
 *  otherwise, returns 0 and *e is not changed
 */
char rexdd_check_CT(rexdd_comp_table_t *CT, uint32_t lvl, rexdd_edge_t *edge1, rexdd_edge_t *edge2, rexdd_edge_t *e)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(CT->table, "Empty computing table");

    uint_fast64_t hash = rexdd_hash_edges(lvl, edge1, edge2, primes[CT->size_index]);

    if (CT->table[hash].edgeA != 0) {
        /*
         *  Non-empty slot. Check if match
         */
        if (CT->table[hash].lvl == lvl && 
            rexdd_edges_are_equal(edge1, CT->table[hash].edge1) &&
            rexdd_edges_are_equal(edge2, CT->table[hash].edge2)) 
        {
            rexdd_set_edge(e,
                    CT->table[hash].edgeA->label.rule,
                    CT->table[hash].edgeA->label.complemented,
                    CT->table[hash].edgeA->label.swapped,
                    CT->table[hash].edgeA->target);
            return 1;
        } 
    }
    /*
     *  Empty slot, which means this pair is new
     */
    return 0;
}
/****************************************************************************
 *
 *  Insert a unpacked node and corresponding edge *e into 
 *  computing table for AND operation
 */
void rexdd_cache_CT(rexdd_comp_table_t *CT, uint32_t lvl, rexdd_edge_t *edge1, rexdd_edge_t *edge2, rexdd_edge_t *e)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(e, "Null target edge");
    /*
     *  Check if we should enlarge the table.
     *  when number of entries greater than half of size
     */
    if (CT->num_entries > primes[CT->size_index] / 2) {
        CT->size_index++;
        uint_fast64_t new_size = primes[CT->size_index] ? primes[CT->size_index] : (0x01ul << 60);
        CT->table = realloc(CT->table, new_size*sizeof(rexdd_edge_in_ct));
        for (uint_fast64_t i=primes[CT->size_index-1]; i<new_size; i++) {
            CT->table[i].lvl = 0;
            CT->table[i].edge1 = 0;
            CT->table[i].edge2 = 0;
            CT->table[i].edgeA = 0;
        }
    }
    uint_fast64_t hash = rexdd_hash_edges(lvl, edge1, edge2, primes[CT->size_index]);
    /*
     *  Empty slot, need malloc
     */
    if (CT->table[hash].edgeA == 0) {
        CT->table[hash].edge1 = malloc(sizeof(rexdd_edge_t));
        CT->table[hash].edge2 = malloc(sizeof(rexdd_edge_t));    
        CT->table[hash].edgeA = malloc(sizeof(rexdd_edge_t));
        CT->num_entries++;
    } else {
        /*
         * Non-empty slot, so we overwrite it
         */
        rexdd_sanity1(CT->table[hash].edge1 && CT->table[hash].edge2, "Null edges pair in computing table");
    }
    CT->table[hash].lvl = lvl;
    rexdd_set_edge(CT->table[hash].edge1,
                edge1->label.rule,
                edge1->label.complemented,
                edge1->label.swapped,
                edge1->target);
    rexdd_set_edge(CT->table[hash].edge2,
                edge2->label.rule,
                edge2->label.complemented,
                edge2->label.swapped,
                edge2->target);
    rexdd_set_edge(CT->table[hash].edgeA,
                e->label.rule,
                e->label.complemented,
                e->label.swapped,
                e->target);
}


void rexdd_sweep_CT(rexdd_comp_table_t *CT, rexdd_nodeman_t *M)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(M, "Null nodeman");
    
    for (uint_fast64_t i=0; i<primes[CT->size_index]; i++) {
        if (CT->table[i].edgeA == 0) {
            continue;
        } else {
            if (rexdd_is_packed_marked(rexdd_get_packed_for_handle(M, CT->table[i].edge1->target))
                || rexdd_is_packed_marked(rexdd_get_packed_for_handle(M, CT->table[i].edge2->target))
                || rexdd_is_packed_marked(rexdd_get_packed_for_handle(M, CT->table[i].edgeA->target)))
            {
                CT->table[i].lvl = 0;
                free(CT->table[i].edge1);
                CT->table[i].edge1 = 0;
                free(CT->table[i].edge2);
                CT->table[i].edge2 = 0;
                free(CT->table[i].edgeA);
                CT->table[i].edgeA = 0;
            }
        }
    }// done for loop
}