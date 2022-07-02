
set(EXES
  packing
)

enable_testing()

add_executable(packing packing.c)
target_link_libraries(packing PUBLIC RexDD)
target_include_directories(packing PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)
add_test(NAME packing COMMAND packing)
