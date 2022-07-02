cmake_minimum_required(VERSION 3.11)

project(RexDD VERSION 0.1)

if (MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wunused)
endif()

add_subdirectory(src)
add_subdirectory(tests)

