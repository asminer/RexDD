cmake_minimum_required(VERSION 3.19)

project(RexDD VERSION 0.1)

if (MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wunused -std=c99)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

