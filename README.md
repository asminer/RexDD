# RexDD

Rule on Edge, Complemented, Swapped, Decision Diagram library

This is an open-source library for Binary Decision Diagrams
and variants. implemented in C.
Long edges may be annotated with different reduction rules,
allowing for a single forest to contain BDDs, zero-suppressed BDDs,
and other reductions mixed together.
Edges may be complemented to invert the output of a function,
or swapped to invert the next variable.

# Required for building

- cmake version 3.19 or later
- latest c compiler
- graphviz (optional for BDDs visualization)

# Building on Unix-like or Windows systems

This can be built for QBDD, FBDD, ZBDD, QBDD with complement bit (CQBDD), 
QBDD with complement and swap bit (CSQBDD), FBDD with complement bit (CFBDD), 
FBDD with complement and swap bit (CSFBDD), ESRBDD, CESRBDD, RexBDD (default).

1. Run
```cmake . --preset=debug```
and/or
```cmake . --preset=release```
to set up debugging/release build directories for RexBDD (default). 

1. cd into build-debug or build-release

2. Run ```make```

3. To run regression tests, run ```ctest```

# Building a shared library

Static library file is built by default under ```./build-release/src``` if set ```--preset=release```, 
use the last line in ```CMakeLists.txt``` in ```src```, then following the same steps 
will create the shared library under the same path.