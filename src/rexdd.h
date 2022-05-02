#ifndef REXDD_H
#define REXDD_H

#include <stdbool.h>

/********************************************************************

    Edge encoding.

    We use 64 bits for an edge, as an unsigned long integer,
    with bits divided out for each portion of the edge:


    Upper 8 bits:
        5 bits for edge rule -> 32 edge rules max
        1 complement bit
        1 swap bit  (decide what this means for terminals)
        1 terminal/nonterminal bit

    Lower 56 bits:
        nonterminal node handle (means a max of 2^56 > 10^16 nodes)
        or
        terminal node handle
            strip off a few of the 56 bits for type information (int/float)
            and use bottom 32-bits (or more?) for the terminal value.


    These definitions and macros should be used when
    manipulating edges, in case the internal representation changes.

********************************************************************/

/// Edge (with rules) to a node within a forest.
typedef unsigned long rexdd_edge;


/**
    Reduction rules (TBD):
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


// TBD: define these as macros, use all caps or not?

rexdd_rule rexdd_get_rule(rexdd_edge e);

bool rexdd_is_comp(rexdd_edge e);
bool rexdd_is_swap(rexdd_edge e);

bool rexdd_is_terminal(rexdd_edge e);
bool rexdd_is_nonterminal(rexdd_edge e);

bool rexdd_is_int_terminal(rexdd_edge e);
bool rexdd_is_float_terminal(rexdd_edge e);

int rexdd_get_int_terminal(rexdd_edge e);
float rexdd_get_float_terminal(rexdd_edge e);


/********************************************************************

    Nodes.

    Note that this is the interface;
    inside a forest, nodes may be stored completely differently.

********************************************************************/

/**
    Structure used to create, fetch nodes from a forest.
*/
struct rexdd_node {
    rexdd_edge low;
    rexdd_edge high;
    unsigned level;
    unsigned char error;    // not sure yet if we need this
};

typedef struct rexdd_node* rexdd_nodep;


/********************************************************************

    Forest settings.

    There are not many settings for now, but to keep the
    interface stable, all forest settings should be incorporated
    into this structure.

    Current settings:

        num_levels:     number of variables.  In the struct for
                        consistency.

    Probable additions:

        terminal_type:  booleans, integers, reals, or custom?

        switching on/off various rules, complement bits, swap bits

********************************************************************/

struct rexdd_forest_settings {
    unsigned num_levels;

    // TBD
};

typedef struct rexdd_forest_settings* rexdd_forest_settingsp;



/**
    Fill in defaults for the struct of forest settings.
    This makes it easier to modify one or two settings
    and use defaults for the rest.

    @param  L   Number of variables.
    @param  s   Settings; will be overwritten (unless null).

 */
void rexdd_default_forest_settings(unsigned L, rexdd_forest_settingsp s);


/********************************************************************

    BDD Forest.

    Not sure if we want to expose this or not.

    Benefits of exposing:
        will simplify the interface, as users can grab
        items they need (e.g., current #nodes, peak #nodes, etc.)

        could include an error code within the struct, for
        the result of the last operation
        (malloc error, etc.)

        Can have stack allocated forests.


    Benefits of not exposing:
        really discourages users from tinkering with the struct


    We could have a public version of the struct, and
    a function that returns a (const) pointer to it.
    something like

        struct rexdd_forest_readable {
            unsigned long current_nodes;
            unsigned long peak_nodes;
            // etc.
        };

        const struct rexdd_forest_readable* rexdd_read_forest(rexdd_forestp F)

********************************************************************/

/*
    TBD: define struct rexdd_forest here, or not?
*/

typedef struct rexdd_forest* rexdd_forestp;


/**
    Allocate and initialize a forest.
    @param  s           Pointer to forest settings to use.
*/
rexdd_forestp rexdd_create_forest(const rexdd_forest_settingsp s);

/**
    Free all memory used by a forest.
    @param  F           Pointer to forest struct
*/
void rexdd_destroy_forest(rexdd_forestp F);

/**
    Evaluate the function encoded by an edge.
    @param  F       Pointer to forest struct
    @param  e       Edge encoding the function
    @param  vars    Array of variable assignments,
                    where vars[k] gives the assignment
                    (either 0 or 1 as an integer)
                    to the level k variable.
                    Levels are numbered from 1 (bottom) to num_levels (top).
                    The array must be at least dimension
                    1+num_levels.

    @return     The terminal node reached, as an edge.
                The edge rule will be N,
                the swap bit will be 0,
                the terminal bit will be 1.
*/
rexdd_edge rexdd_evaluate(rexdd_forestp F, rexdd_edge e,
                const unsigned char* vars);


/**
    Add a node to the forest.
    @param  F       Pointer to forest struct
    @param  rule    Rule on the edge to the node
    @param  node    Node to add
    @return An edge that encodes the same function as the node
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
    TBD: locking / unlocking mechanism for root nodes in a forest,
    needed for mark and sweep garbage collection
*/


/*
    TBD:
    apply operations.
*/

#endif
