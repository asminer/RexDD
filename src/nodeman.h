#ifndef NODEMAN_H
#define NODEMAN_H

#include "unpacked.h"

/*
 *  Efficient storage of lots of nodes, for use by a forest.
 *
 */

#define REXDD_PAGESIZE (1024*1024)

/*
 *  Node handle.
 */
typedef uint_fast64_t  rexdd_node_handle;

/*
 *  Packed node, as 192 bits (3 x 64 bits).
 *
 *  A packed node contains the following entries.
 *      next (in unique table)  : 49 bits (1/2 quadrillion max nodes)
 *      mark (for G.C.)         :  1 bit
 *
 *      loch (low child)        : 50 bits (high bit: 0 terminal, 1 nonterm.)
 *      hich (high child)       : 50 bits
 *      loru (low rule)         :  5 bits
 *      hiru (high rule)        :  5 bits
 *
 *      level                   : 29 bits (1/2 billion max levels)
 *      losw (low swap)         :  1 bit
 *      hisw (high swap)        :  1 bit
 *      hico (high complement)  :  1 bit
 *
 *
 *  Use the inlined functions below to access a packed node safely.
 */
typedef struct {
    /* bits 00..48: next
     * bits 49..49: mark
     * bits 50..63: loch (upper 14)
     */
    uint64_t first64;

    /* bits 00..35: loch (lower 36)
     * bits 36..63: hich (upper 28)
     */
    uint64_t second64;

    /* bits 00..21: hich (lower 22)
     * bits 22..26: loru
     * bits 27..31: hiru
     */
    uint32_t third32;

    /* bits 00..28: level
     * bits 29..29: losw
     * bits 30..30: hisw
     * bits 31..31: hico
     */
    uint32_t fourth32;
} rexdd_packed_node_t;

typedef rexdd_packed_node_t* rexdd_packed_node_p;

#define NEXT_MASK   0x0001ffffffffffff
#define MARK_MASK   0x0002000000000000
#define TOP14_MASK  0xfffc000000000000

#define LOW36_MASK  0x0000000fffffffff

#define LOW22_MASK  0x003fffff
#define LORU_MASK   0x07c00000
#define HIRU_MASK   0xf1000000

#define LOW29_MASK  0x1fffffff
#define BIT29_MASK  0x20000000
#define BIT30_MASK  0x40000000
#define BIT31_MASK  0x80000000

// Get the next pointer from a packed node.
static inline uint_fast64_t
rexdd_get_packed_next(const rexdd_packed_node_p N)
{
    return N->first64 & NEXT_MASK;
}

// Set the next pointer in a packed node.
static inline void
rexdd_set_packed_next(rexdd_packed_node_p N, uint64_t nxt)
{
    N->first64 = (N->first64 & ~NEXT_MASK) | (nxt & NEXT_MASK);
}

// Is the packed node marked.
static inline bool
rexdd_is_packed_marked(const rexdd_packed_node_p N)
{
    return N->first64 & MARK_MASK;
}

// Mark a packed node
static inline void
rexdd_mark_packed(rexdd_packed_node_p N)
{
    N->first64 |= MARK_MASK;
}

// Unmark a packed node
static inline void
rexdd_unmark_packed(rexdd_packed_node_p N)
{
    N->first64 &= ~MARK_MASK;
}

// Get the low child from a packed node.
static inline uint64_t
rexdd_get_packed_loch(const rexdd_packed_node_p N)
{
    return ((N->first64 & TOP14_MASK) >> 14)
            |
           (N->second64 & LOW36_MASK);
}

// Set the low child in a packed node.
static inline void
rexdd_set_packed_loch(rexdd_packed_node_p N, uint64_t c)
{
    N->first64 = (N->first64 & ~TOP14_MASK) | ( (c << 14) & TOP14_MASK);
    N->second64 = (N->second64 & ~LOW36_MASK) | (c & LOW36_MASK);
}

// Get the high child from a packed node.
static inline uint64_t
rexdd_get_packed_hich(const rexdd_packed_node_p N)
{
    return ((N->second64 & ~LOW36_MASK) >> 14)
            |
           (N->third32 & LOW22_MASK);
}

// Set the high child in a packed node.
static inline void
rexdd_set_packed_hich(rexdd_packed_node_p N, uint64_t c)
{
    N->second64 = (N->second64 & LOW36_MASK) | ( (c << 14) & ~LOW36_MASK );
    N->third32 = (N->third32 & ~LOW22_MASK) | (c & LOW22_MASK);
}

// Get the low rule in a packed node.
static inline uint8_t
rexdd_get_packed_loru(const rexdd_packed_node_p N)
{
    return (N->third32 & LORU_MASK) >> 22;
}

// Set the low rule in a packed node
static inline void
rexdd_set_packed_loru(rexdd_packed_node_p N, uint32_t r)
{
    N->third32 = (N->third32 & ~LORU_MASK) | ( (r << 22) & LORU_MASK );
}

// Get the high rule in a packed node.
static inline uint8_t
rexdd_get_packed_hiru(const rexdd_packed_node_p N)
{
    return (N->third32 & HIRU_MASK) >> 27;
}

// Set the high rule in a packed node
static inline void
rexdd_set_packed_hiru(rexdd_packed_node_p N, uint32_t r)
{
    N->third32 = (N->third32 & ~HIRU_MASK) | ( (r << 27) & HIRU_MASK );
}

// Get the level from a packed node.
static inline uint_fast32_t
rexdd_get_packed_level(const rexdd_packed_node_p N)
{
    return N->fourth32 & LOW29_MASK;
}

// Set the level of a packed node.
static inline void
rexdd_set_packed_level(rexdd_packed_node_p N, uint32_t L)
{
    N->fourth32 = (N->fourth32 & ~LOW29_MASK) | (L & LOW29_MASK);
}


// Get the low swap flag from a packed node.
static inline bool
rexdd_get_packed_losw(const rexdd_packed_node_p N)
{
    return N->fourth32 & BIT29_MASK;
}

// Set the low swap flag in a packed node.
static inline void rexdd_set_packed_losw(rexdd_packed_node_p N, uint32_t s)
{
    N->fourth32 = (N->fourth32 & ~BIT29_MASK) | ((s << 29) & BIT29_MASK);
}

// Get the high swap flag from a packed node.
static inline bool
rexdd_get_packed_hisw(const rexdd_packed_node_p N)
{
    return N->fourth32 & BIT30_MASK;
}

// Set the high swap flag in a packed node.
static inline void rexdd_set_packed_hisw(rexdd_packed_node_p N, uint32_t s)
{
    N->fourth32 = (N->fourth32 & ~BIT30_MASK) | ((s << 30) & BIT30_MASK);
}

// Get the high complement flag from a packed node.
static inline bool
rexdd_get_packed_hico(const rexdd_packed_node_p N)
{
    return N->fourth32 & BIT31_MASK;
}

// Set the high complement flag in a packed node.
static inline void rexdd_set_packed_hico(rexdd_packed_node_p N, uint32_t s)
{
    N->fourth32 = (N->fourth32 & ~BIT31_MASK) | (s << 31);
}


// Check if two nodes are duplicates.
static inline bool
rexdd_are_packed_duplicates(const rexdd_packed_node_p P,
        const rexdd_packed_node_p Q)
{
    return  (P->second64 == Q->second64) &&
            (P->third32 == Q->third32) &&
            (P->fourth32 == Q->fourth32) &&
            ((P->first64 & TOP14_MASK) == (Q->first64 & TOP14_MASK));
}


/*********************************************************************
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 ********************************************************************/


/*
 *  TBD - design this struct
 */
typedef struct {
    /*  Array for node storage. */
    /*  first unallocated */
    /*  list of free slots */
    /*  total number of free slots */
    /*  pointer (index) for various lists of pages */
} rexdd_nodepage_t;

typedef rexdd_nodepage_t* rexdd_nodepage_p;

/*
 *  TBD - design this struct
 *      should be visible b/c it will be needed for macros below.
 */
typedef struct {
    /* Array of pointers to rexdd_nodepage  */
    /* array size */
    /* last used in the array */
    /* list of pages with free slots, in increasing order of #free slots */
} rexdd_nodeman_t;

typedef rexdd_nodeman_t* rexdd_nodeman_p;

/*
 *  Initialize a node manager.
 *
 *  TBD - what settings here?
 *      maybe max number of levels?
 *      maybe max number of nodes?
 *
 *  return 0 on success...
 */
int rexdd_create_nodeman(rexdd_nodeman_p M);

/*
 *  Destroy and free memory used by a node manager.
 */
int rexdd_destroy_nodeman(rexdd_nodeman_p M);

/*
 *  TBD - compaction?
 *      need a renumbering array
 */

/*
 *  Get a free node handle
 */
rexdd_node_handle   rexdd_new_handle(rexdd_nodeman_p M);

/*
 *  Return a node handle to the free store
 */
void    rexdd_free_handle(rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Fill in an unpacked node from a node handle.
 *
 *  return 0 on success...
 */
int rexdd_unpack_handle(rexdd_unpacked_node_p u,
        const rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Fill in a packed node at the specified handle.
 */
int rexdd_pack_handle(rexdd_nodeman_p M, rexdd_node_handle h,
        const rexdd_unpacked_node_p u);


/*
 *  Get the next handle in a chain.
 *      (probably as a macro or static inline function (allowed since C99))
 */
rexdd_node_handle GET_NEXT_HANDLE(const rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Set the next handle in a chain.
 *      (probably as a macro)
 */
rexdd_node_handle SET_NEXT_HANDLE(rexdd_nodeman_p M, rexdd_node_handle h,
        rexdd_node_handle nxt);

/*
 *  Is a node marked?
 *      (probably as a macro)
 */
uint64_t  IS_HANDLE_MARKED(const rexdd_nodeman_p M, rexdd_node_handle h);

/*
 *  Mark a node.
 *      (probably as a macro)
 */
void    MARK_HANDLE(rexdd_nodeman_p M, rexdd_node_handle h);

/*
 *  Unmark a node.
 *      (probably as a macro)
 */
void    UNMARK_HANDLE(rexdd_nodeman_p M, rexdd_node_handle h);


/*
 *  Compute a raw hash value for a node.
 *      (inline)
 */
static inline uint64_t HASH_HANDLE(const rexdd_nodeman_p M, rexdd_node_handle h)
{
    // TBD
    return 0;
}

#endif
