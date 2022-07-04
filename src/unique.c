
#include "unique.h"

#include <stdlib.h> // for malloc
#include <string.h>

// #define TEST_PRIMES

// #define TRACE_ON

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

#ifdef TEST_PRIMES

int main()
{
    unsigned i;
    uint_fast64_t f;
    for (i=0; primes[i]; i++) {
        fprintf(stderr, "Checking %llu...", primes[i]);
        if (0==primes[i] % 2) {
            fprintf(stderr, " composite! (divisible by 2)\n");
            continue;
        }
        for (f=3; f*f <= primes[i]; f+=2) {
            if (0==primes[i] % f) {
                fprintf(stderr, " composite! (divisible by %llu)\n", f);
                break;
            }
        }
        if (f*f > primes[i]) fprintf(stderr, " PRIME\n");
    }
    return 0;
}

#endif

/****************************************************************************
 *
 * Helper: zero an array
 *
 */
static inline void zero_array(rexdd_node_handle_t *A, uint_fast64_t n)
{
    while (n) {
        A[--n] = 0;
    }
}

/****************************************************************************
 *
 *  Initialize a unique table
 *
 */
void rexdd_init_UT(rexdd_unique_table_t *T, rexdd_nodeman_t *M)
{
    rexdd_sanity1(T, "Null unique table");
    rexdd_sanity1(M, "Null node manager");

    T->M = M;
    T->size = primes[0];
    T->table = malloc(T->size * sizeof(rexdd_node_handle_t));
    rexdd_check1(T->table, "Malloc fail in rexdd_init_UT");
    zero_array(T->table, T->size);

    rexdd_check1(T->table, "malloc fail in unique table");

    T->num_entries = 0;
    T->prev_size_index = 0;
    T->size_index = 0;
    T->enlarge = primes[0];
}

/****************************************************************************
 *
 *  Free memory for a unique table.
 *  The underlying node manager is not touched.
 */
void rexdd_free_UT(rexdd_unique_table_t *T)
{
    rexdd_sanity1(T, "Null unique table");
    if (0==T->table) return;
    free(T->table);

    // Set everything to sane values
    T->table = 0;
    T->num_entries = 0;
    T->size = 0;
    T->size_index = 0;
    T->prev_size_index = 0;
    T->enlarge = 0x01ul << 60;
}


/****************************************************************************
 *
 * Helper: collect hash table chains into a single list; return front
 *
 */
static uint_fast64_t rexdd_table_to_list(rexdd_unique_table_t *T)
{
    if (0==T) return 0;

    uint_fast64_t front = 0;
    rexdd_packed_node_t *packed = 0;
    uint_fast64_t i, chain;
    for (i=0; i<T->size; i++) {
        while (T->table[i]) {
            chain = T->table[i];
            packed = rexdd_get_packed_for_handle(T->M, chain);
            T->table[i] = rexdd_get_packed_next(packed);
            rexdd_set_packed_next(packed, front);
            front = chain;
        }
    } // for i
    T->num_entries = 0;
    return front;
}

/****************************************************************************
 *
 * Helper: re-hash a list
 *
 */
static void rexdd_rehash_list(uint_fast64_t list, rexdd_unique_table_t *T)
{
    rexdd_sanity1(T, "Null unique table");

    rexdd_packed_node_t *packed = 0;
    uint_fast64_t next, hash;
    while (list) {
        packed = rexdd_get_packed_for_handle(T->M, list);

        // Save next pointer, before we overwrite it
        next = rexdd_get_packed_next(packed);

        // Compute new hash
        hash = rexdd_hash_packed(packed, T->size);

        // Add to the front of the new list
        rexdd_set_packed_next(packed, T->table[hash]);
        T->table[hash] = list;

        // Advance
        list = next;
        T->num_entries++;
    }
}

/****************************************************************************
 *
 *  Add a handle to the unique table.
 *  If unique, returns the same handle;
 *  otherwise, returns the handle of the duplicate and recycles
 *      the given handle.
 *  In either case, the returned node becomes the front entry
 *  of the hash chain.
 */
rexdd_node_handle_t rexdd_insert_UT(rexdd_unique_table_t *T, rexdd_node_handle_t h)
{
    rexdd_sanity1(T, "Null unique table");
    rexdd_sanity1(T->table, "Empty unique table");

    /*
     * Check if we should enlarge the table.  TBD
     */
    if (T->num_entries > T->enlarge) {
        uint_fast64_t list = rexdd_table_to_list(T);

        // enlarge
        T->size_index++;
        T->size = primes[T->size_index];
        T->table = realloc(T->table, T->size * sizeof(rexdd_node_handle_t));
        rexdd_check1(T->table, "Realloc fail in rexdd_insert_UT");
        zero_array(T->table, T->size);
        T->enlarge = primes[T->size_index+1] ? T->size : (0x01ul << 60);

        rexdd_rehash_list(list, T);
    }

    /*
     * Determine hash for the node
     */
    rexdd_packed_node_t *node = rexdd_get_packed_for_handle(T->M, h);
    uint_fast64_t hash = rexdd_hash_packed(node, T->size);

#ifdef TRACE_ON
    fprintf(stderr, "UT insert\n");
    fprintf(stderr, "    handle: %llu\n", h);
    fprintf(stderr, "      hash: %llu\n", hash);
#endif

    /*
     *  Special, and hopefully common, case: empty chain.
     *  Which means the node is new.
     */
    if (0==T->table[hash]) {
        T->num_entries++;
        T->table[hash] = h;
        // node's next should already be 0
#ifdef TRACE_ON
        fprintf(stderr, "UT insert: empty chain, returning %llu\n", h);
#endif
        return h;
    }

    /*
     *  Non-empty chain.  Check the chain for duplicates.
     */
#ifdef TRACE_ON
    uint_fast64_t i;
    fprintf(stderr, "    start chain: ");
    for (i = T->table[hash];
        i;
        i = rexdd_get_packed_next(rexdd_get_packed_for_handle(T->M, i)))
    {
        fprintf(stderr, "%llu, ", i);
    }
    fprintf(stderr, ".\n");
#endif
    uint_fast64_t currhand = T->table[hash];
    rexdd_packed_node_t *prevnode = 0;
    rexdd_packed_node_t *currnode = 0;


    while (currhand) {
        currnode = rexdd_get_packed_for_handle(T->M, currhand);
        if (!rexdd_are_packed_duplicates(node, currnode)) {
            currhand = rexdd_get_packed_next(currnode);
            prevnode = currnode;
            continue;
        }
        /*
         *  We found a duplicate.  Move it to the front.
         */
        if (prevnode) {
            rexdd_set_packed_next(prevnode, rexdd_get_packed_next(currnode));
            rexdd_set_packed_next(currnode, T->table[hash]);
            T->table[hash] = currhand;
        }
        /*
         *  Recycle the given handle.
         */
        rexdd_nodeman_reuse(T->M, h);

#ifdef TRACE_ON
        fprintf(stderr, "    found chain: ");
        for (i = T->table[hash];
            i;
            i = rexdd_get_packed_next(rexdd_get_packed_for_handle(T->M, i)))
        {
            fprintf(stderr, "%llu, ", i);
        }
        fprintf(stderr, ".\n");
        fprintf(stderr, "UT insert: duplicate, returning %llu\n", currhand);
#endif
        return currhand;
    }

    /*
     *  No duplicates in the chain.  Add the new node to the front.
     */
    T->num_entries++;
    rexdd_set_packed_next(node, T->table[hash]);
    T->table[hash] = h;

#ifdef TRACE_ON
    fprintf(stderr, "    added chain: ");
    for (i = T->table[hash];
        i;
        i = rexdd_get_packed_next(rexdd_get_packed_for_handle(T->M, i)))
    {
        fprintf(stderr, "%llu, ", i);
    }
    fprintf(stderr, ".\n");
    fprintf(stderr, "UT insert: new item, returning %llu\n", h);
#endif

    return h;
}


/****************************************************************************
 *
 *  Remove all unmarked nodes from the unique table.
 *      @param  T   Unique table to modify
 */
void rexdd_sweep_UT(rexdd_unique_table_t *T)
{
    rexdd_sanity1(T, "Null unique table");
    rexdd_sanity1(T->table, "Empty unique table");

    /*
     *  For each chain, traverse and keep only the marked items.
     */
    uint_fast64_t i, h;
    rexdd_packed_node_t *prev;
    rexdd_packed_node_t *curr;
    T->num_entries = 0;
    for (i=0; i<T->size; i++) {
        prev = 0;
        h = T->table[i];
        while (h) {
            curr = rexdd_get_packed_for_handle(T->M, h);
            if (rexdd_is_packed_marked(curr)) {
                /* attach us to the chain */
                T->num_entries++;
                if (prev) {
                    rexdd_set_packed_next(prev, h);
                } else {
                    T->table[i] = h;
                }
                prev = curr;
            }
            h = rexdd_get_packed_next(curr);
        }
        /*
         *  Done traversing; null terminate the new chain
         */
        if (prev) {
            rexdd_set_packed_next(prev, 0);
        } else {
            T->table[i] = 0;
        }
    } // for i

    /*
     * Check if we should shrink the table.  TBD
     */

}

/****************************************************************************
 *
 *  Build chain length histogram, for debugging / performance measurement.
 *      @param  T           Unique table to dump
 *      @param  hist        Histogram, where hist[i] is the number of
 *                          chains of length i.
 *      @param  hsize       Size of histogram; any outliers will be ignored.
 *
 */
void rexdd_histogram_UT(const rexdd_unique_table_t *T,
        uint_fast64_t* hist, unsigned hsize)
{
    uint_fast64_t h, i;
    for (i=0; i<hsize; i++) hist[i] = 0;
    if (0==T) {
        return;
    }
#ifndef NDEBUG
    uint_fast64_t tlen = 0;
#endif
    for (i=0; i<T->size; i++) {
        unsigned clen = 0;
        for (h = T->table[i];
            h;
            h = rexdd_get_packed_next(rexdd_get_packed_for_handle(T->M, h)))
        {
            ++clen;
        }
        if (clen < hsize) ++hist[clen];
#ifndef NDEBUG
        tlen += clen;
#endif
    }
    rexdd_sanity1(tlen == T->num_entries, "UT entry count is wrong");
}

/****************************************************************************
 *
 *  Dump a unique table, in human-readable format, for debugging purposes.
 *      @param  fout        Where to dump
 *      @param  T           Unique table to dump
 *      @param  show_nodes  If true, display the nodes;
 *                          otherwise just shows handles.
 *
 */
void rexdd_dump_UT(FILE* fout, const rexdd_unique_table_t *T, bool show_nodes)
{
    if (0==T) {
        fprintf(fout, "Null unique table\n");
        return;
    }
    uint_fast64_t i, h;
    fprintf(fout, "---------+\n");
    unsigned col = 0;
    const rexdd_packed_node_t *cur_p;
    rexdd_unpacked_node_t cur_u;

    char nodebuf[256];
    for (i=0; i<T->size; i++) {
        fprintf(fout, "%8lx |--", (unsigned long) i);
        h = T->table[i];
        col = 0;
        while (h) {
            cur_p = rexdd_get_packed_for_handle(T->M, h);

            // Build char buffer of node to display

            if (show_nodes) {
                rexdd_packed_to_unpacked(cur_p, &cur_u);
                fprintf(fout, "-> { Level %u, ", (unsigned) cur_u.level);
                rexdd_fprint_edge(fout, cur_u.edge[0]);
                fputs(", ", fout);
                rexdd_fprint_edge(fout, cur_u.edge[1]);
                fprintf(fout, "}\n         |  ");
            } else {
                snprintf(nodebuf, 256, "-> %lx", (unsigned long) h);
                col += strlen(nodebuf);
                if (col > 60) {
                    fprintf(fout, "\n         |  %s", nodebuf);
                    col = 0;
                } else {
                    fprintf(fout, "%s", nodebuf);
                }
            }

            // Next in the chain

            h = rexdd_get_packed_next(cur_p);
        }
        fprintf(fout, "\n---------+\n");
    }
}


