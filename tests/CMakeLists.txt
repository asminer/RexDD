
#
# Test executables
#
set(EXES
  check_pack
  check_np
  check_ut
  check_reduce
  check_bdds
  check_bench_marks
)

foreach(testexe ${EXES})
  add_executable(${testexe} ${testexe}.c)
  target_link_libraries(${testexe} PUBLIC RexDD)
  target_include_directories(${testexe} PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
  )
endforeach()

#
# Allow quoted test names with spaces
#
cmake_policy(SET CMP0110 NEW)

#
# Add the tests to run.
# Not automated, in case tests take arguments / input files.
#

add_test("Node packing test"   check_pack)
add_test("Node page test"      check_np)
add_test("Unique table test"   check_ut)
add_test("Reduce edge test" check_reduce)
add_test("QBDD/FBDD/ZBDD/RexBDD eval test" check_bdds)
add_test("Benchmarks test"  check_bench_marks)
