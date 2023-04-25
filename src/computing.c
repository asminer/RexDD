
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
 *  Helper: Get a free slot in table
 *
 */
uint_fast64_t rexdd_ct_free_slot(rexdd_comp_table_t* CT)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(CT->tableR || CT->tableA, "Slot request from unallocated table");
    rexdd_sanity1(CT->num_entries<CT->size, "Slot request from full table");

    CT->num_entries++;

    if (CT->free_list) {
        uint_fast64_t slot = CT->free_list-1;
        CT->free_list = (CT->type=='R') ? CT->tableR[slot].node->third32 : CT->tableA[slot].next;
        return slot;
    }
    return CT->first_unalloc++;
}

/****************************************************************************
 *
 *  Helper: collect hash table chains into a single list; return front
 *  Retruns the fornt of list in reverse order (comparing to the order in *handles)
 *
 */
static uint_fast64_t rexdd_ct_to_list(rexdd_comp_table_t *CT)
{
    if (!CT) return 0;

    uint_fast64_t front = 0;
    uint_fast64_t i, chain;
    for (i=0; i<CT->size; i++) {
        while (CT->handles[i]) {
            chain = CT->handles[i];
            if (CT->type=='R'){
                CT->handles[i] = rexdd_get_packed_next(CT->tableR[chain].node);
                rexdd_set_packed_next(CT->tableR[chain].node, front);
            } else {
                CT->handles[i] = CT->tableA[chain].next;
                CT->tableA[chain].next = front;
            }
            front = chain;
        }
    } // end for i
    CT->num_entries = 0;
    return front;
}

/****************************************************************************
 *
 * Helper: re-hash a list
 *
 */
static void rexdd_rehash_ct_list(uint_fast64_t list, rexdd_comp_table_t *CT)
{
    rexdd_sanity1(CT, "Null computing table");

    rexdd_packed_node_t *packed = 0;
    uint_fast64_t next, hash;
    while (list) {
        /*
         *  If it's R computing table
         */
        packed = CT->tableR[list].node;
        next = rexdd_get_packed_next(packed);
        hash = rexdd_hash_packed(packed, CT->size);
        rexdd_set_packed_next(packed, CT->handles[hash]);
        
        /*
         *   If it's A computing table
         *      TBD
         */

        CT->handles[hash] = list;
        list = next;
        CT->num_entries++;
    }
}

/****************************************************************************
 *
 *  Initialize a computing table
 *
 */
void rexdd_init_CT(rexdd_comp_table_t *CT, char type)
{
    rexdd_sanity1(CT, "Null unique table");

    CT->size = primes[0];
    CT->type = type;
    if (type == 'R') {
        CT->tableR = malloc(CT->size * sizeof(rexdd_reduce_edge_in_ct));     // malloc and set zeros
        rexdd_check1(CT->tableR, "Malloc tableR fail in rexdd_init_CT");
        for (uint_fast64_t i=0; i<CT->size; i++) {
            CT->tableR[i].node = 0;
            CT->tableR[i].edge = 0;
        }
        CT->tableA = 0;
    } else if (type == 'A') {
        CT->tableA = malloc(CT->size * sizeof(rexdd_AND_edge_in_ct));     // malloc and set zeros
        rexdd_check1(CT->tableA, "Malloc tableA fail in rexdd_init_CT");
        for (uint_fast64_t i=0; i<CT->size; i++) {
            CT->tableA[i].edge1 = 0;
            CT->tableA[i].edge2 = 0;
            CT->tableA[i].edgeA = 0;
            CT->tableA[i].next = 0;
        }
        CT->tableR = 0;
    } else {
        printf("Unknown type of computing table!\n");
        exit(1);
    }
    CT->handles = calloc(CT->size, sizeof(uint_fast64_t));
    rexdd_check1(CT->handles, "Malloc handles fail in rexdd_init_CT");
    
    CT->num_entries = 0;
    CT->first_unalloc = 0;
    CT->free_list = 0;
    CT->prev_size_index = 0;
    CT->size_index = 0;
    CT->enlarge = primes[0];
}

/****************************************************************************
 *
 *  Free memory for a computing table.
 */
void rexdd_free_CT(rexdd_comp_table_t *CT)
{
    rexdd_sanity1(CT, "Null unique table");

    if (!CT->tableR && !CT->tableA) return;
    for (uint_fast64_t i=0; i<CT->size; i++) {
        if (CT->type == 'R') {
            if (CT->tableR[i].node) free(CT->tableR[i].node);
            CT->tableR[i].node = 0;
            if (CT->tableR[i].edge) free(CT->tableR[i].edge);
            CT->tableR[i].edge = 0;
        } else {
            if (CT->tableA[i].edge1) free(CT->tableA[i].edge1);
            CT->tableA[i].edge1 = 0;
            if (CT->tableA[i].edge2) free(CT->tableA[i].edge2);
            CT->tableA[i].edge2 = 0;
            if (CT->tableA[i].edgeA) free(CT->tableA[i].edgeA);
            CT->tableA[i].edgeA = 0;
            CT->tableA[i]. next = 0;
        }
    }
    (CT->type == 'R')? free(CT->tableR) : free(CT->tableA);
    CT->tableR = 0;
    CT->tableA = 0;

    if (CT->handles) free(CT->handles);
    CT->handles = 0;

    CT->num_entries = 0;
    CT->size = 0;
    CT->first_unalloc = 0;
    CT->free_list = 0;
    CT->size_index = 0;
    CT->prev_size_index = 0;
    CT->enlarge = 0x01ul << 60;
    CT->type = 0;
}

/****************************************************************************
 *
 *  Check entry in a computing table.
 *  If cached, returns 1 and corresponding edge is set to *e;
 *  otherwise, returns 0 and *e is not changed
 */
char rexdd_check_reduce_CT(rexdd_comp_table_t *CT, rexdd_unpacked_node_t *node, rexdd_edge_t *e)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(CT->tableR, "Empty computing table");
    rexdd_sanity1(node, "Null unpacked node");

    /*
     *  Determine the hash for the node
     *  Note: results of hash function always less than CT->size (as the second parameter)
     */
    rexdd_packed_node_t packedNode;
    rexdd_unpacked_to_packed(node, &packedNode);
    uint_fast64_t hash = rexdd_hash_packed(&packedNode, CT->size);
    uint_fast64_t front = CT->handles[hash];

    /*
     *  Empty chain, not cached return 0;
     */
    if (0 == CT->tableR[front].node) return 0;

    /*
     *  Non-empty chain. Check the chain for duplicates
     */
    uint_fast64_t currhand = front;
    rexdd_packed_node_t* currnode = 0;
    rexdd_packed_node_t* prevnode = 0;

    while (currhand) {
        currnode = CT->tableR[currhand].node;
        if (!rexdd_are_packed_duplicates(&packedNode, currnode)) {
            currhand = rexdd_get_packed_next(currnode);     // next pointer in table
            prevnode = currnode;
            continue;
        }
        /*
         *  We found a duplicate at currhand. Move it to the front.
         *  Set the edge as result
         */
        if (prevnode) {
            rexdd_set_packed_next(prevnode, rexdd_get_packed_next(currnode));
            rexdd_set_packed_next(currnode, CT->handles[hash]);
            CT->handles[hash] = currhand;
        }
        rexdd_set_edge(e,
                    CT->tableR[currhand].edge->label.rule,
                    CT->tableR[currhand].edge->label.complemented,
                    CT->tableR[currhand].edge->label.swapped,
                    CT->tableR[currhand].edge->target);
        return 1;
    }

    /*
     *  No duplicates in the chain, not cached return 0;
     */
    return 0;
}

/****************************************************************************
 *
 *  Insert a unpacked node and corresponding edge *e into computing table
 */
void rexdd_insert_reduce_CT(rexdd_comp_table_t *CT, rexdd_unpacked_node_t *node, rexdd_edge_t *e)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(CT->tableR, "Empty computing table");
    rexdd_sanity1(node, "Null unpacked node");

    /*
     *  Check if we should enlarge the table
     */
    if (CT->num_entries == CT->enlarge-1) {
        // let's convert it to list then enlarge
        uint_fast64_t list = rexdd_ct_to_list(CT);

        CT->size_index++;
        CT->size = primes[CT->size_index];
        CT->tableR = realloc(CT->tableR, CT->size * sizeof(rexdd_reduce_edge_in_ct));    // not change the old contents
        rexdd_sanity1(CT->tableR, "Realloc table fail in rexdd_insert_CT");
        // zero out the new slots
        uint_fast64_t i;
        for (i=CT->size; i>primes[CT->size_index-1]; i--) {
            CT->tableR[i-1].node = 0;
            CT->tableR[i-1].edge = 0;

        }
        free(CT->handles);
        CT->handles = calloc(CT->size, sizeof(uint_fast64_t));  // new *handles is set to zeros
        rexdd_sanity1(CT->handles, "Calloc handles fail in rexdd_insert_CT");
        CT->enlarge = primes[CT->size_index+1] ? CT->size : (0x01ul << 60);
        
        rexdd_rehash_ct_list(list, CT);
    }

    /* Checked before, this is definitely a new entry not cached  */

    /*
     *  Get a free or unalloc slot in table to store entry;
     *  Set the node and edge at slot
     */
    uint_fast64_t slot;
    slot = rexdd_ct_free_slot(CT);
    if (CT->tableR[slot].node == 0) {
        // this is an unalloc slot, so malloc one
        CT->tableR[slot].node = malloc(sizeof(rexdd_packed_node_t));
        rexdd_check1(CT->tableR[slot].node, "Malloc table->node fail in rexdd_insert_CT");
        CT->tableR[slot].edge = malloc(sizeof(rexdd_edge_t));
        rexdd_check1(CT->tableR[slot].edge, "Malloc table->edge fail in rexdd_insert_CT");
    }
    rexdd_unpacked_to_packed(node, CT->tableR[slot].node);
    rexdd_set_edge(CT->tableR[slot].edge,
                    e->label.rule,
                    e->label.complemented,
                    e->label.swapped,
                    e->target);

    /*
     *  Determine the hash for the node
     */
    rexdd_packed_node_t packedNode;
    rexdd_unpacked_to_packed(node, &packedNode);
    uint_fast64_t hash = rexdd_hash_packed(&packedNode, CT->size);
    uint_fast64_t front = CT->handles[hash];
    /*
     *  Move this node to the front
     */
    rexdd_set_packed_next(CT->tableR[slot].node, front);
    CT->handles[hash] = slot;
    CT->num_entries++;
    return;
}

void rexdd_sweep_CT(rexdd_comp_table_t *CT, rexdd_nodeman_t *M)
{
    rexdd_sanity1(CT, "Null computing table");
    rexdd_sanity1(M, "Null nodeman");
    /*
     *      TBD
     */


}