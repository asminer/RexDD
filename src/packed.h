#ifndef PACKED_H
#define PACKED_H

#include "unpacked.h"

/****************************************************************************
 *
 *  Packed node, as 192 bits (3 x 64 bits).
 *
 *  A packed node contains the following entries.
 *      next (in unique table)  : 49 bits (1/2 quadrillion max nodes)
 *      mark (for G.C.)         :  1 bit
 *
 *      loch (low child)        : 50 bits
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


/****************************************************************************
 *
 * Get the next pointer from a packed node.
 * static inlined for speed.
 *      @param      N packed node
 *      @return     N's next pointer.
 */
static inline uint_fast64_t
rexdd_get_packed_next(const rexdd_packed_node_t *N)
{
    static const uint64_t   NEXT_MASK   = (0x01ul << 49) - 1;  // bits 0..48
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
rexdd_set_packed_next(rexdd_packed_node_t *N, uint64_t nxt)
{
    static const uint64_t   NEXT_MASK   = (0x01ul << 49) - 1;  // bits 0..48
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
rexdd_is_packed_marked(const rexdd_packed_node_t *N)
{
    static const uint64_t   MARK_MASK   = 0x01ul << 49;   // bit 49
    return N->first64 & MARK_MASK;
}

/****************************************************************************
 *
 *  Mark the specified packed node.
 *  static inlined for speed.
 *      @param      N   packed node to mark.
 */
static inline void
rexdd_mark_packed(rexdd_packed_node_t *N)
{
    static const uint64_t   MARK_MASK   = 0x01ul << 49;   // bit 49
    N->first64 |= MARK_MASK;
}

/****************************************************************************
 *
 *  Unmark the specified packed node.
 *  static inlined for speed.
 *      @param      N   packed node to mark.
 */
static inline void
rexdd_unmark_packed(rexdd_packed_node_t *N)
{
    static const uint64_t   MARK_MASK   = 0x01ul << 49;   // bit 49
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
rexdd_unpacked_to_packed(const rexdd_unpacked_node_t *uN, rexdd_packed_node_t *pN)
{
    static const uint64_t   TOP14_MASK  = ~((0x01ul << 50)-1);  // bits 50..63
    static const uint64_t   LOW36_MASK  = (0x01ul << 36) - 1; // bits 0..35

    static const uint32_t   LOW22_MASK  = (0x01 << 22) - 1; // bits 0..21
    static const uint32_t   LORU_MASK   = ((0x01 << 5) - 1) << 22;    // bits 22..26
    static const uint32_t   HIRU_MASK   = ((0x01 << 5) - 1) << 27;    // bits 27..31
    static const uint32_t   LOW29_MASK  = (0x01ul << 29) - 1; // bits 0..29
    static const uint32_t   BIT29_MASK  = 0x01ul << 29;
    static const uint32_t   BIT30_MASK  = 0x01ul << 30;
    static const uint32_t   BIT31_MASK  = 0x01ul << 31;

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
 *  Get the level of a packed node.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 */
static inline uint_fast32_t
rexdd_unpack_level(const rexdd_packed_node_t *pN)
{
    // bits 0..29
    static const uint32_t   LOW29_MASK  = (0x01ul << 29) - 1;

    return pN->fourth32 & LOW29_MASK;
}


/****************************************************************************
 *
 *  Get the low edge label from a packed node.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 *      @param      el  Edge label to fill.
 */
static inline void
rexdd_unpack_low_edge(const rexdd_packed_node_t *pN, rexdd_edge_label_t *el)
{
    // bits 22..26
    static const uint32_t   LORU_MASK   = ((0x01 << 5) - 1) << 22;
    // bit 29
    static const uint32_t   BIT29_MASK  = 0x01ul << 29;

    el->rule = (pN->third32 & LORU_MASK) >> 22;
    el->swapped = pN->fourth32 & BIT29_MASK;
    el->complemented = 0;
}

/****************************************************************************
 *
 *  Get the low edge target node from a packed node.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 */
static inline rexdd_node_handle_t
rexdd_unpack_low_child(const rexdd_packed_node_t *pN)
{
    // bits 50..63
    static const uint64_t   TOP14_MASK  = ~((0x01ul << 50)-1);
    // bits 0..35
    static const uint64_t   LOW36_MASK  = (0x01ul << 36) - 1;

    return  ((pN->first64 & TOP14_MASK) >> 14)
            |
            (pN->second64 & LOW36_MASK);
}

/****************************************************************************
 *
 *  Get the high edge label from a packed node.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 *      @param      el  Edge label to fill.
 */
static inline void
rexdd_unpack_high_edge(const rexdd_packed_node_t *pN, rexdd_edge_label_t *el)
{
    // bits 27..31
    static const uint32_t   HIRU_MASK   = ((0x01 << 5) - 1) << 27;
    static const uint32_t   BIT30_MASK  = 0x01ul << 30;
    static const uint32_t   BIT31_MASK  = 0x01ul << 31;

    el->rule = (pN->third32 & HIRU_MASK) >> 27;
    el->swapped = pN->fourth32 & BIT30_MASK;
    el->complemented = pN->fourth32 & BIT31_MASK;
}

/****************************************************************************
 *
 *  Get the high edge target node from a packed node.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 */
static inline rexdd_node_handle_t
rexdd_unpack_high_child(const rexdd_packed_node_t *pN)
{
    // bits 0..35
    static const uint64_t   LOW36_MASK  = (0x01ul << 36) - 1;
    // bits 0..21
    static const uint32_t   LOW22_MASK  = (0x01 << 22) - 1;

    return  ((pN->second64 & ~LOW36_MASK) >> 14)
            |
            (pN->third32 & LOW22_MASK);
}


/****************************************************************************
 *
 *  Fill in an unpacked node from a packed one.
 *  static inlined for speed.
 *      @param      pN  Packed node to read from.
 *      @param      uN  Unpacked node to fill.
 */
static inline void
rexdd_packed_to_unpacked(const rexdd_packed_node_t *pN, rexdd_unpacked_node_t *uN)
{
    rexdd_unpack_low_edge(pN, &(uN->edge[0].label));
    uN->edge[0].target = rexdd_unpack_low_child(pN);

    rexdd_unpack_high_edge(pN, &(uN->edge[1].label));
    uN->edge[1].target = rexdd_unpack_high_child(pN);

    uN->level = rexdd_unpack_level(pN);
}


/****************************************************************************
 *
 *  Fill in a packed node as appropriate for recycling into a free list.
 *  static inlined for speed.
 *      @param      N           Packed node to update.
 *      @param      next_free   Next "pointer" in free list.
 */
static inline void
rexdd_recycle_packed(rexdd_packed_node_t *N, uint_fast32_t next_free)
{
    N->first64 = 0;
    N->second64 = 0;
    N->third32 = next_free;
    N->fourth32 = 0;
}


/****************************************************************************
 *
 *  Check if a packed node is in use or not.
 *  static inlined for speed.
 *      @param      N           Packed node
 */
static inline bool
rexdd_is_packed_in_use(const rexdd_packed_node_t *N)
{
    return N->fourth32;
}


/****************************************************************************
 *
 *  Check if two packed nodes are duplicates.
 *  (This is faster than unpacking and comparing unpacked nodes.)
 *  static inlined for speed.
 */
static inline bool
rexdd_are_packed_duplicates(const rexdd_packed_node_t *P,
        const rexdd_packed_node_t *Q)
{
    static const uint64_t   TOP14_MASK  = ~((0x01ul << 50)-1);  // bits 50..63

    return  (P->second64 == Q->second64) &&
            (P->third32 == Q->third32) &&
            (P->fourth32 == Q->fourth32) &&
            ((P->first64 & TOP14_MASK) == (Q->first64 & TOP14_MASK));
}


/****************************************************************************
 *
 *  Hash a packed node, modulo m.
 *  static inlined for speed.
 */
static inline uint_fast64_t
rexdd_hash_packed(const rexdd_packed_node_t *P, uint_fast64_t m)
{
    uint64_t h;
    if (0 == (m & ~((0x01ul << 32) - 1)) ) {
        /*
         * m fits in 32 bits ->
         *      anything mod m fits in 32 bits ->
         *      can multiply by 32-bit chunks
         */
        h = P->second64 % m;

        h = ( (h << 32) | P->third32 ) % m;
        h = ( (h << 32) | P->fourth32 ) % m;

        return (( h << 14) | (P->first64 >> 50)) % m;
    }

    if (0 == (m & ~ ((0x01ul << 16) - 1)) ) {
        /*
         * m fits in 48 bits ->
         *      anything mod m fits in 48 bits ->
         *      can multiply by 16-bit chunks
         */
        h = P->second64 % m;

        h = ( ( h << 16) | ( P->third32 >> 16) ) % m;
        h = ( ( h << 16) | ( P->third32 & 0xffff) ) % m;

        h = ( ( h << 16) | ( P->fourth32 >> 16) ) % m;
        h = ( ( h << 16) | ( P->fourth32 & 0xffff) ) % m;

        return (( h << 14) | (P->first64 >> 50)) % m;
    }

    /*
     * m fits in 56 bits (or we don't care about overflows any more) ->
     *      anything mod m fits in 56 bits ->
     *      can multiply by 8-bit chunks
     */

    h = P->second64 % m;

    h = ( ( h << 8 ) | ( P->third32 >> 24) ) % m;
    h = ( ( h << 8 ) | (( P->third32 >> 16) & 0xff) ) % m;
    h = ( ( h << 8 ) | (( P->third32 >>  8) & 0xff) ) % m;
    h = ( ( h << 8 ) | ( P->third32 & 0xff) ) % m;

    h = ( ( h << 8 ) | ( P->fourth32 >> 24) ) % m;
    h = ( ( h << 8 ) | (( P->fourth32 >> 16) & 0xff) ) % m;
    h = ( ( h << 8 ) | (( P->fourth32 >>  8) & 0xff) ) % m;
    h = ( ( h << 8 ) | ( P->fourth32 & 0xff) ) % m;

    h = ( ( h << 8 ) | ( P->first64 >> 56) ) % m;
    return ( ( h << 6 ) | (( P->first64 >> 50) & 0x3f ) ) % m;
}


#endif
