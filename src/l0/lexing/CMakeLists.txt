add_library(lexing token.h token.cpp lexer.h lexer.cpp)

target_link_libraries(lexing common)
