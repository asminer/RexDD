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

cmake version 3.19 or later
latest c compiler
graphviz (optional for BDDs visualization)

# Building on Unix-like or Windows systems

This can be built for QBDD, FBDD, ZBDD, QBDD with complement bit (CQBDD), 
QBDD with complement and swap bit (CSQBDD), FBDD with complement bit (CFBDD), 
FBDD with complement and swap bit (CSFBDD), ESRBDD, CESRBDD, RexBDD (default).

1. Run
```cmake . --preset=debug```
and/or
```cmake . --preset=release```
to set up debugging/release build directories for RexBDD (default). 
Run
```cmake . --preset=debug -D*BDD=ON```
and/or
```cmake . --preset=release -D*BDD=ON```
to set up debugging/release build directories for *BDD. For example, 
run
```cmake . --preset=debug -DQBDD=ON```
to set up debugging build directories for QBDD.

2. cd into build-debug or build-release

3. Run ```make```

4. To run regression tests, run ```ctest```

5. To change build for another type of BDDs when there is already one 
build directory,
run ```rm CMakeCache.txt; rm -rf CMakeFiles``` in the same build directory, 
then cd into rexdd and run ```cmake . --preset=debug -D*BDD=ON``` for *BDD

# Building a shared library

Static library file is built by default under ./build-release/src, 
use the last line in CMakeLists.txt in src, then following the same steps 
will create the shared library under the same path.