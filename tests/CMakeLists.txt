
#
# Test executables
#
set(EXES
  packing
  check_ut
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
# Add the tests to run.
# Not automated, in case tests take arguments / input files.
#

add_test(NAME packing COMMAND packing)
add_test(NAME check_ut COMMAND check_ut)
