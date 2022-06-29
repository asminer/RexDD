What's in each header file:
============================================================

error.h     :   function to be called for library errors.
                Eventually - users can provide their own to
                use instead of the default.

forest.h    :   forest interface

nodeman.h   :   efficient storage for large collections of nodes.
                used by a forest.

nodepage.h  :   a single "page" of nodes, used by nodeman.

packed.h    :   packed node structure and helper (inline) functions.

unique.h    :   unique table for detecting duplicate nodes.
                used by a forest.

unpacked.h  :   interface for "unpacked" nodes



General conventions:
============================================================

  Type naming conventions:
    struct tags have names ending in _s (when they are used).
    typedef'd types (structs, etc) have names ending in _t.

    no typedefs for pointers because const does the unexpected.
