
#
# Tests that are run with no arguments
#
set(NOARG_TESTS
  packing
)

foreach(testexe ${NOARG_TESTS})
  add_executable(${testexe} ${testexe}.c)
  target_link_libraries(${testexe} PUBLIC RexDD)
  target_include_directories(${testexe} PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
  )
  add_test(NAME ${testexe} COMMAND ${testexe})
endforeach()
