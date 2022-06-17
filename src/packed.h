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
#define HIRU_MASK   0xf8000000

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

/****************************************************************************
 *
 *  Fill in a packed node from an unpacked one.
 *  Sets next to 0 and clears the marked bit.
 *  static inlined for speed.
 *      @param      uN  Unpacked node to read from.
 *      @param      pN  Packed node to fill.
 */
static inline void
rexdd_unpacked_to_packed(const rexdd_unpacked_node_p uN, rexdd_packed_node_p pN)
{
    pN->first64 =
        ( (uN->edge[0].target << 14) & TOP14_MASK);

    pN->second64 =
        ( (uN->edge[1].target << 14) & ~LOW36_MASK)
        |
        (uN->edge[0].target & LOW36_MASK);

    pN->third32 =
        ( (uN->edge[1].label.rule << 27) & HIRU_MASK )
        |
        ( (uN->edge[0].label.rule << 22) & LORU_MASK )
        |
        (uN->edge[1].target & LOW22_MASK);

    pN->fourth32 =
        (uN->edge[1].label.complemented * BIT31_MASK)
        |
        (uN->edge[1].label.swapped * BIT30_MASK)
        |
        (uN->edge[0].label.swapped * BIT29_MASK)
        |
        (uN->level & LOW29_MASK);
}


/****************************************************************************
 *
 *  Fill in an unpacked node from a packed one.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 *      @param      uN  Unpacked node to fill.
 */
static inline void
rexdd_packed_to_unpacked(const rexdd_packed_node_p pN, rexdd_unpacked_node_p uN)
{
    uN->edge[0].label.rule = (pN->third32 & LORU_MASK) >> 22;
    uN->edge[0].label.swapped = pN->fourth32 & BIT29_MASK;
    uN->edge[0].label.complemented = 0;
    uN->edge[0].target =
        ((pN->first64 & TOP14_MASK) >> 14)
        |
        (pN->second64 & LOW36_MASK);

    uN->edge[1].label.rule = (pN->third32 & HIRU_MASK) >> 27;
    uN->edge[1].label.swapped = pN->fourth32 & BIT30_MASK;
    uN->edge[1].label.complemented = pN->fourth32 & BIT31_MASK;
    uN->edge[1].target =
        ((pN->second64 & ~LOW36_MASK) >> 14)
        |
        (pN->third32 & LOW22_MASK);

    uN->level = pN->fourth32 & LOW29_MASK;
}


/****************************************************************************
 *
 *  Fill in a packed node as appropriate for recycling into a free list.
 *  static inlined for speed.
 *      @param      N           Packed node to update.
 *      @param      next_free   Next "pointer" in free list.
 */
static inline void
rexdd_recycle_packed(rexdd_packed_node_p N, uint_fast32_t next_free)
{
    N->first64 = 0;
    N->second64 = 0;
    N->third32 = next_free;
    N->fourth32 = 0;
}


/****************************************************************************
 *
 *  Check if two packed nodes are duplicates.
 *  (This is faster than unpacking and comparing unpacked nodes.)
 *  static inlined for speed.
 */
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
