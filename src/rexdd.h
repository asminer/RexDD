
/*
  TBD: probably want to define the forest struct here,
  so users can access things like 'number of levels'.
  Or we can have macros for that, e.g.,
    num_levels(forestptr F)

  TBD: mark and sweep GC stuff, maybe
    lock_node(forestptr F, rexdd_edge)
    unlock_node(forestptr F, rexdd_edge)
  for managing the list of root nodes.
*/
typedef struct forest* forestptr;

typedef struct {
  // TBD: allowed edge rules
  char allow_complement;
  char allow_swap;
} forest_settings;

/**
  Initialize a forest.
    @param  F           Pointer to forest struct
    @param  num_levels  Number of variables, numbered 1..num_levels,
                        with 0 for terminal nodes, 1 for the bottom-most
                        level, and num_levels for the top-most level.
    @param  s           Pointer to forest settings to use.
                        If null, then default settings are used.
*/
void init_forest(forestptr F, unsigned num_levels, const forest_settings* s);

/**
  Free all memory used by a forest.
    @param  F           Pointer to forest struct
*/
void done_forest(forestptr F);

/**
  Edge, with edge annotations and destination node.
  Upper 8 bits:
    5 bits for edge rule -> 32 edge rules max
    1 complement bit
    1 swap bit  (decide what this means for terminals)
    1 terminal/nonterminal bit

  Lower 56 bits:
    nonterminal node handle (means a max of 2^56 > 10^16 nodes)
    terminal 32-bit int or float
*/
typedef unsigned long rexdd_edge;

/*
  Probably want edge macros:

    is_terminal(rexdd_edge e)
    is_nonterminal(rexdd_edge e)

    is_int_terminal(rexdd_edge e)
    is_float_terminal(rexdd_edge e)

    get_int_terminal(rexdd_edge e) -> integer
    get_float_terminal(rexdd_edge r) -> float

    get_nonterminal(rexdd_edge e) -> target node

    get_rule(rexdd_edge e)  -> unsigned char
    get_comp(rexdd_edge e)  -> 0 or non-zero
    get_swap(rexdd_edge e)  -> 0 or non-zero

    build_int_terminal(forestptr F, int term) -> rexdd_edge
    build_float_terminal(forestptr F, float term) -> rexdd_edge
*/

/**
  Evaluate the function encoded by an edge.
    @param  F       Pointer to forest struct
    @param  e       Edge encoding the function
    @param  vars    Array of variable assignments,
                    where vars[k] gives the assignment
                    (either 0 or 1 as an integer)
                    to the level k variable.
                    The array must be at least dimension
                    1+num_levels.

    @return The terminal node reached, as an edge.
*/
rexdd_edge  eval_edge(forestptr F, rexdd_edge e, const char* vars);

typedef struct {
  unsigned level;
  rexdd_edge low;
  rexdd_edge high;
} rexdd_node;


/**
  TBD: should there be an edge rule also?

  Add a node to the forest.
    @param  F       Pointer to forest struct
    @param  node    Node to add
    @return An edge that encodes the same function as the node
            (or should it be an edge to a node?)
*/
rexdd_edge reduce_node(forestptr F, const rexdd_node* node);

/**
  Get node information from an edge.
    @param  F       Pointer to forest struct
    @param  e       Edge we care about. Edge annotations are ignored.
    @param  node    Pointer to struct, will be filled in.
*/
void get_node(forestptr F, rexdd_edge e, rexdd_node* node);



/*
  TBD:
  apply operations.
*/
