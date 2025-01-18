add_executable(l0c main.cpp)

target_link_libraries(l0c LLVM lexing parsing semantics generation types)
