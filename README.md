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

cmake version 3.11 or later

# Building on Unix-like systems

1. Run
```cmake . --preset=debug```
and/or
```cmake . --preset=release```
to set up debugging/release build directories.

2. cd into build-debug or build-release

3. Run ```make```

4. To run regression tests, run ```ctest```
