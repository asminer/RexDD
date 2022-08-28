cmake_minimum_required(VERSION 3.19)

project(RexDD VERSION 0.1)

option(QBDD "Build for QBDD")
message("QBDD is ${QBDD}")

option(CQBDD "Build for CQBDD")
message("QBDD with complement bit is ${CQBDD}")

option(CSQBDD "Build for CSQBDD")
message("QBDD with complement and swap bit is ${CSQBDD}")

option(FBDD "Build for FBDD")
message("FBDD is ${FBDD}")

option(CFBDD "Build for CFBDD")
message("FBDD with complement bit is ${CFBDD}")

option(CSFBDD "Build for CSFBDD")
message("FBDD with complement and swap bit is ${CSFBDD}")

option(ZBDD "Build for ZBDD")
message("ZBDD is ${ZBDD}")

option(ESRBDD "Build for ESRBDD")
message("ESRBDD is ${ESRBDD}")

option(CESRBDD "Build for CESRBDD")
message("CESRBDD is ${CESRBDD}")

if (QBDD)
  add_definitions(-DQBDD)
  message("select to build for QBDD")
elseif (FBDD)
  add_definitions(-DFBDD)
  message("select to build for FBDD")
elseif (ZBDD)
  add_definitions(-DZBDD)
  message("select to build for ZBDD")
elseif (ESRBDD)
  add_definitions(-DESRBDD)
  message("select to build for ESRBDD")
else()
  add_definitions(-DREXBDD)
  message("select to build for REXBDD")
endif()

if (MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wunused)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

