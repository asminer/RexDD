#include <stdbool.h>

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
typedef struct rexdd_forest* rexdd_forestp;

typedef struct {
    // TBD: allowed edge rules
    char allow_complement;
    char allow_swap;
} rexdd_forest_settings;

/**
    Initialize a forest.
    @param  F           Pointer to forest struct
    @param  num_levels  Number of variables, numbered 1..num_levels,
                        with 0 for terminal nodes, 1 for the bottom-most
                        level, and num_levels for the top-most level.
    @param  s           Pointer to forest settings to use.
                        If null, then default settings are used.
*/
void rexdd_init_forest(rexdd_forestp F, unsigned num_levels, const rexdd_forest_settings* s);

/**
    Free all memory used by a forest.
    @param  F           Pointer to forest struct
*/
void rexdd_done_forest(rexdd_forestp F);

/**
    Edge, with edge annotations and destination node.
    Upper 8 bits:
        5 bits for edge rule -> 32 edge rules max
        1 complement bit
        1 swap bit  (decide what this means for terminals)
        1 terminal/nonterminal bit

    Lower 56 bits:
        nonterminal node handle (means a max of 2^56 > 10^16 nodes)
        terminal 32-bit int or float (toggle 33rd bit for int and float)
*/
typedef unsigned long rexdd_edge;

/**
    Reduction rules:
        N, X, LZ, LN, HZ, HN, ELZ, ELN, EHZ, EHN, ALZ, ALN, AHZ, AHN
*/
typedef unsigned char rexdd_rule;

enum rexdd_reduction_rule {
    N = 0,
    X = 1,
    LZ = 2,
    LN = 3,
    HZ = 4,
    HN = 5,
    ELZ = 6,
    ELN = 7,
    EHZ = 8,
    EHN = 9,
    ALZ = 10,
    ALN = 11,
    AHZ = 12,
    AHN = 13
};

/*
    Probably want edge macros:

        get_rule(rexdd_edge e) -> unsigned char

        is_comp(rexdd_edge e)
        is_swap(rexdd_edge e)

        is_terminal(rexdd_edge e)
        is_nonterminal(rexdd_edge e)

        is_int_terminal(rexdd_edge e)
        is_float_terminal(rexdd_edge e)

        get_int_terminal(rexdd_edge e) -> integer
        get_float_terminal(rexdd_edge r) -> float

        get_nonterminal(rexdd_edge e) -> target node

        build_int_terminal(forestptr F, int term) -> rexdd_edge
        build_float_terminal(forestptr F, float term) -> rexdd_edge
*/

rexdd_rule rexdd_get_rule(rexdd_edge e);

bool rexdd_is_comp(rexdd_edge e);
bool rexdd_is_swap(rexdd_edge e);

bool rexdd_is_terminal(rexdd_edge e);
bool rexdd_is_nonterminal(rexdd_edge e);

bool rexdd_is_int_terminal(rexdd_edge e);
bool rexdd_is_float_terminal(rexdd_edge e);

int rexdd_get_int_terminal(rexdd_edge e);
float rexdd_get_float_terminal(rexdd_edge e);

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
rexdd_edge rexdd_eval_edge(rexdd_forestp F, rexdd_edge e, const char* vars);

struct rexdd_node {
    rexdd_edge low;
    rexdd_edge high;
    unsigned level;
    unsigned char error;
};

typedef struct rexdd_node* rexdd_nodep;

/**
    Add a node to the forest.
    @param  F       Pointer to forest struct
    @param  rule    Rule on the edge to the node
    @param  node    Node to add
    @return An edge that encodes the same function as the node
            (or should it be an edge to a node?)
*/
rexdd_edge rexdd_reduce_edge(rexdd_forestp F, rexdd_reduction_rule rule,
        const rexdd_nodep node);

/**
    Get node information from an edge.
    @param  F       Pointer to forest struct
    @param  e       Edge we care about. Edge annotations are ignored.
    @param  node    Pointer to struct, will be filled in.
*/
void rexdd_get_node(rexdd_forestp F, rexdd_edge e, rexdd_nodep node);

/*
    TBD: locking / unlocking mechanism for root nodes in a forest
*/

/*
    TBD:
    apply operations.
*/
