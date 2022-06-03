#include "rexdd.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

rexdd_rule rexdd_get_rule(rexdd_edge e)
{
    rexdd_rule rule = e >> 59;
    assert(0 <= rule && rule <= 13);
    return rule;
}

bool rexdd_is_comp(rexdd_edge e)
{
    return 0x0400000000000000 & e;
}

bool rexdd_is_swap(rexdd_edge e)
{
    return 0x0200000000000000 & e;
}

bool rexdd_is_terminal(rexdd_edge e)
{
    return 0x0100000000000000 & e;
}

bool rexdd_is_nonterminal(rexdd_edge e)
{
    return !is_terminal(e);
}

bool rexdd_is_int_terminal(rexdd_edge e)
{
    assert(is_terminal(e));
    return 0x0000000100000000 & e;
}

bool rexdd_is_float_terminal(rexdd_edge e)
{
    assert(is_terminal(e));
    return !is_int_terminal(e);
}

int rexdd_get_int_terminal(rexdd_edge e)
{
    assert(is_int_terminal(e));
    return 0x00000000FFFFFFFF & e;
}

float rexdd_get_float_terminal(rexdd_edge e)
{
    assert(is_float_terminal(e));
    return 0x00000000FFFFFFFF & e;
}
