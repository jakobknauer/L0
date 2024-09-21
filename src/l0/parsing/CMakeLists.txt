add_library(parsing parser.cpp parser.h)

target_link_libraries(parsing ast lexing common)
