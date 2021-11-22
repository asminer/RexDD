#include "rexdd.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

rexdd_rule get_rule(rexdd_edge e)
{
  rexdd_rule rule = e >> 59;
  assert(0 <= rule && rule <= 13);
  return rule;
}

bool is_comp(rexdd_edge e)
{
  return 0x0400000000000000 & e;
}

bool is_swap(rexdd_edge e)
{
  return 0x0200000000000000 & e;
}

bool is_terminal(rexdd_edge e)
{
  return 0x0100000000000000 & e;
}

bool is_nonterminal(rexdd_edge e)
{
  return !is_terminal(e);
}

bool is_int_terminal(rexdd_edge e)
{
  assert(is_terminal(e));
  return 0x0000000100000000 & e;
}

bool is_float_terminal(rexdd_edge e)
{
  assert(is_terminal(e));
  return !is_int_terminal(e);
}

int get_int_terminal(rexdd_edge e)
{
  assert(is_int_terminal(e));
  return 0x00000000FFFFFFFF & e;
}

float get_float_terminal(rexdd_edge e)
{
  assert(is_float_terminal(e));
  return 0x00000000FFFFFFFF & e;
}
