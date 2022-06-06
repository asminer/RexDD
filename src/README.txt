What's in each header file:
============================================================

forest.h    :   forest interface

nodeman.h   :   efficient storage for large collections of nodes.
                used by a forest.

packed.h    :   packed node structure and helper (inline) functions.

unique.h    :   unique table for detecting duplicate nodes.
                used by a forest.

unpacked.h  :   interface for "unpacked" nodes



General conventions:
============================================================

  For typedefs:
      structs and other types have names ending in _t.
      pointers to structs have names ending in _p.



