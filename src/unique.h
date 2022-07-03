#ifndef UNIQUE_H
#define UNIQUE_H

/*
 *  Unique table.
 */

#include "nodeman.h"


/****************************************************************************
 *
 *  Unique (hash) table of packed nodes.
 *
 *  Each hash table entry is a chain of node handles.
 *  The next node handle is stored in the packed node.
 *
 *  The unique table (UT) grows and shrinks dynamically
 *  with the number of elements in the table, according
 *  to a fixed sequence of prime sizes.
 *
 */
typedef struct {
    rexdd_nodeman_t *M;      /* node manager that stores the packed nodes. */
    rexdd_node_handle_t* table;
    uint_fast64_t size;
    uint_fast64_t num_entries;
    uint_fast64_t enlarge;  // #entries required to enlarge
    unsigned size_index;
    unsigned prev_size_index;
} rexdd_unique_table_t;

typedef rexdd_unique_table_t* rexdd_unique_table_p;


/****************************************************************************
 *
 *  Initialize a unique table
 *
 */
void rexdd_init_UT(rexdd_unique_table_t *T, rexdd_nodeman_t *M);


/****************************************************************************
 *
 *  Free memory for a unique table.
 *  The underlying node manager is not touched.
 */
void rexdd_free_UT(rexdd_unique_table_t *T);


/****************************************************************************
 *
 *  Add a handle to the unique table.
 *  If unique, returns the same handle;
 *  otherwise, returns the handle of the duplicate and recycles
 *      the given handle.
 *  In either case, the returned node becomes the front entry
 *  of the hash chain.
 */
rexdd_node_handle_t rexdd_insert_UT(rexdd_unique_table_t *T, rexdd_node_handle_t h);


/****************************************************************************
 *
 *  Remove all unmarked nodes from the unique table.
 *      @param  T   Unique table to modify
 */
void rexdd_sweep_UT(rexdd_unique_table_t *T);


/****************************************************************************
 *
 *  Build chain length histogram, for debugging / performance measurement.
 *      @param  T           Unique table to dump
 *      @param  hist        Histogram, where hist[i] is the number of
 *                          chains of length i.
 *      @param  hsize       Size of histogram; any outliers will be ignored.
 *
 */
void rexdd_histogram_UT(const rexdd_unique_table_t *T, uint_fast64_t* hist, unsigned hsize);


/****************************************************************************
 *
 *  Dump a unique table, in human-readable format, for debugging purposes.
 *      @param  fout        Where to dump
 *      @param  T           Unique table to dump
 *      @param  show_nodes  If true, display the nodes;
 *                          otherwise just shows handles.
 *
 */
void rexdd_dump_UT(FILE* fout, const rexdd_unique_table_t *T, bool show_nodes);


#endif
