
set(SOURCES
  error.c
  forest.c
  functions.c
  helpers.c
  nodeman.c
  nodepage.c
  unique.c
  unpacked.c
  parser.c
)

configure_file(config.h.in config.h)

add_library(RexDD ${SOURCES})
