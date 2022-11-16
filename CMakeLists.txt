cmake_minimum_required(VERSION 3.19)

project(RexDD VERSION 0.1)

option(QBDD "Build for QBDD")
message("QBDD is ${QBDD}")

option(CQBDD "Build for CQBDD")
message("CQBDD is ${CQBDD}")

option(CSQBDD "Build for CSQBDD")
message("CSQBDD is ${CSQBDD}")

option(FBDD "Build for FBDD")
message("FBDD is ${FBDD}")

option(CFBDD "Build for CFBDD")
message("CFBDD is ${CFBDD}")

option(CSFBDD "Build for CSFBDD")
message("CSFBDD is ${CSFBDD}")

option(ZBDD "Build for ZBDD")
message("ZBDD is ${ZBDD}")

option(ESRBDD "Build for ESRBDD")
message("ESRBDD is ${ESRBDD}")

option(CESRBDD "Build for CESRBDD")
message("CESRBDD is ${CESRBDD}")

option(SQBDD "Build for SQBDD")
message("SQBDD is ${SQBDD}")

option(SFBDD "Build for SFBDD")
message("SFBDD is ${SFBDD}")

if (QBDD)
  add_definitions(-DQBDD)
  message("select to build for QBDD")
elseif (FBDD)
  add_definitions(-DFBDD)
  message("select to build for FBDD")
elseif (ZBDD)
  add_definitions(-DZBDD)
  message("select to build for ZBDD")
elseif (CQBDD)
  add_definitions(-DC_QBDD)
  message("select to build for QBDD with complement bit")
elseif (CSQBDD)
  add_definitions(-DCS_QBDD)
  message("select to build for QBDD with complement and swap bit")
elseif (CFBDD)
  add_definitions(-DC_FBDD)
  message("select to build for FBDD with complement bit")
elseif (CSFBDD)
  add_definitions(-DCS_FBDD)
  message("select to build for FBDD with complement and swap bit")
elseif (CESRBDD)
  add_definitions(-DCESRBDD)
  message("select to build for CESRBDD")
elseif (ESRBDD)
  add_definitions(-DESRBDD)
  message("select to build for ESRBDD")
elseif (SQBDD)
  add_definitions(-DS_QBDD)
  message("select to build for QBDD with swap bit")
elseif (SFBDD)
  add_definitions(-DS_FBDD)
  message("select to build for FBDD with swap bit")
else()
  add_definitions(-DREXBDD)
  message("select to build for REXBDD")
endif()

if (MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wunused -std=c99)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

