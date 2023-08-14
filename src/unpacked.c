
#include "unpacked.h"

#include <string.h>
#include <stdio.h>

// TBD - should this be private, in unpacked.c?
static const char* rexdd_rule_name[] = {
    "EL0",
    "AL0",
    "EL1",
    "AL1",
    "EH0",
    "AH0",
    "EH1",
    "AH1",
    "X",
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
    static const uint64_t low24 = (0x01ul << 24) - 1;

    const char* label = rexdd_rule_name[e.label.rule];
    if (0==label) label = "null";

    if (rexdd_is_terminal(e.target)) {
        // Terminal.
        snprintf(buffer, len, "<%s,%c,%c,T%lx>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (unsigned long) rexdd_terminal_value(e.target)
        );
    } else {
        // Non-terminal
        rexdd_node_handle_t et = rexdd_nonterminal_handle(e.target);
        snprintf(buffer, len, "<%s,%c,%c,N%x:%x>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (uint32_t) (et >> 24),
            (uint32_t) (et & low24)
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
    static const uint64_t low24 = (0x01ul << 24) - 1;

    const char* label = rexdd_rule_name[e.label.rule];
    if (0==label) label = "null";

    if (rexdd_is_terminal(e.target)) {
        // Terminal.
        fprintf(fout, "<%s,%c,%c,T%lx>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (unsigned long) rexdd_terminal_value(e.target)
        );
    } else {
        // Non-terminal
        rexdd_node_handle_t et = rexdd_nonterminal_handle(e.target);
        fprintf(fout, "<%s,%c,%c,N%x:%06x>",
            label,
            e.label.complemented ? 'c' : '_',
            e.label.swapped ? 's' : '_',
            (uint32_t) (et >> 24),
            (uint32_t) (et & low24)
        );
    }
}

