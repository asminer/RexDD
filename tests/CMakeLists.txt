
#
# Test executables
#
set(EXES
  check_pack
  check_np
  check_ut
  check_forest
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

add_test(NAME check_pack COMMAND check_pack)
add_test(NAME check_ut COMMAND check_ut)
add_test(NAME check_forest COMMAND check_forest)
