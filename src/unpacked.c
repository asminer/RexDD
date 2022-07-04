
#include "unpacked.h"

#include <string.h>
#include <stdio.h>


// TBD - should this be private, in unpacked.c?
static const char* rexdd_rule_name[] = {
    "N",
    "X",
    "L0",
    "L1",
    "H0",
    "H1",
    "EL0",
    "EL1",
    "EH0",
    "EH1",
    "AL0",
    "AL1",
    "AH0",
    "AH1",
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

/****************************************************************************
 *
 *  Write an edge, in human-readable format, to a char buffer.
 *
 *  @param  buffer      Character buffer to write into.  Can be null.
 *  @param  len         Length of the buffer, so we don't write past the end.
 *                      Can be 0, e.g., if buffer is null.
 *  @param  e           Edge to 'display'
 *
 */
void rexdd_snprint_edge(char* buffer, unsigned len, rexdd_edge_t e)
{
    static const uint64_t bit50 = 0x01ul << 49;
    static const uint64_t low24 = (0x01ul << 24) - 1;
    static const uint64_t low32 = (0x01ul << 32) - 1;

    const char* label = rexdd_rule_name[e.label.rule];
    if (0==label) label = "null";

    if (e.target & bit50) {
        // Terminal.
        snprintf(buffer, len, "<%s,%c,%c,T%x>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (uint32_t) (e.target & low32)
        );
    } else {
        // Non-terminal
        snprintf(buffer, len, "<%s,%c,%c,N%x:%06x>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (uint32_t) (e.target >> 24),
            (uint32_t) (e.target & low24)
        );
    }
}


/****************************************************************************
 *
 *  Write an edge, in human-readable format, to a FILE.
 *
 *  @param  fout        File to write.
 *  @param  e           Edge to 'display'
 *
 */
void rexdd_fprint_edge(FILE* fout, rexdd_edge_t e)
{
    static const uint64_t bit50 = 0x01ul << 49;
    static const uint64_t low24 = (0x01ul << 24) - 1;
    static const uint64_t low32 = (0x01ul << 32) - 1;

    const char* label = rexdd_rule_name[e.label.rule];
    if (0==label) label = "null";

    if (e.target & bit50) {
        // Terminal.
        fprintf(fout, "<%s,%c,%c,T%x>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (uint32_t) (e.target & low32)
        );
    } else {
        // Non-terminal
        fprintf(fout, "<%s,%c,%c,N%x:%06x>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (uint32_t) (e.target >> 24),
            (uint32_t) (e.target & low24)
        );
    }
}

