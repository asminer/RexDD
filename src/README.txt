What's in each header file:
============================================================

computing.h :   computing table (n, edge1, edge2):edgeA for 
                logic operations.

error.h     :   function to be called for library errors.
                Eventually - users can provide their own to
                use instead of the default.

forest.h    :   forest interface

helpers.h   :   interface for some helper functions.

nodeman.h   :   efficient storage for large collections of nodes.
                used by a forest.

nodepage.h  :   a single "page" of nodes, used by nodeman.

operation.h :   interface for logic operations, including AND, OR,
                XOR, and NOT.

packed.h    :   packed node structure and helper (inline) functions.

parser.h    :   interface for parser reading minterms file.

unique.h    :   unique table for detecting duplicate nodes.
                used by a forest.

unpacked.h  :   interface for "unpacked" nodes



General conventions:
============================================================

  Type naming conventions:
    struct tags have names ending in _s (when they are used).
    typedef'd types (structs, etc) have names ending in _t.

    no typedefs for pointers because const does the unexpected.
