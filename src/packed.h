#ifndef PACKED_H
#define PACKED_H

/****************************************************************************
 *
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
     *
     * Note - for unused nodes, this is the next pointer in the free list.
     */
    uint32_t third32;

    /* bits 00..28: level
     * bits 29..29: losw
     * bits 30..30: hisw
     * bits 31..31: hico
     *
     * Note - if this is zero, then the node is unused.
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


/****************************************************************************
 *
 * Get the next pointer from a packed node.
 * static inlined for speed.
 *      @param      N packed node
 *      @return     N's next pointer.
 */
static inline uint_fast64_t
rexdd_get_packed_next(const rexdd_packed_node_p N)
{
    return N->first64 & NEXT_MASK;
}

/****************************************************************************
 *
 * Set the next pointer in a packed node.
 * static inlined for speed.
 *      @param      N   packed node
 *      @param      nxt what to assign to N.next
 */
static inline void
rexdd_set_packed_next(rexdd_packed_node_p N, uint64_t nxt)
{
    N->first64 = (N->first64 & ~NEXT_MASK) | (nxt & NEXT_MASK);
}

/****************************************************************************
 *
 *  For a given packed node, is it marked?
 *  static inlined for speed.
 *      @param      N   packed node
 *      @return     true, iff N is marked.
 */
static inline bool
rexdd_is_packed_marked(const rexdd_packed_node_p N)
{
    return N->first64 & MARK_MASK;
}

/****************************************************************************
 *
 *  Mark the specified packed node.
 *  static inlined for speed.
 *      @param      N   packed node to mark.
 */
static inline void
rexdd_mark_packed(rexdd_packed_node_p N)
{
    N->first64 |= MARK_MASK;
}

/****************************************************************************
 *
 *  Unmark the specified packed node.
 *  static inlined for speed.
 *      @param      N   packed node to mark.
 */
static inline void
rexdd_unmark_packed(rexdd_packed_node_p N)
{
    N->first64 &= ~MARK_MASK;
}

//
// TBD FROM HERE DOWN - rewrite as packed -> unpacked and vice versa,
// because (1) that's what we'll use anyway and (2) we can simplify some.
//

static inline void
rexdd_unpacked_to_packed(const rexdd_unpacked_node_p uN, rexdd_packed_node_p pN)
{
    // TBD
}

static inline void
rexdd_packed_to_unpacked(const rexdd_packed_node_p pN, rexdd_unpacked_node_p uN)
{
    // TBD
}

static inline void
rexdd_recycle_packed(rexdd_packed_node_p N, uint_fast32_t next_free)
{
    N->first64 = 0;
    N->second64 = 0;
    N->third32 = next_free;
    N->fourth32 = 0;
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



#endif
