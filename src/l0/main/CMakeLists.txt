add_executable(main main.cpp)

target_link_libraries(main LLVM)
target_link_libraries(main lexing)
target_link_libraries(main parsing)
target_link_libraries(main semantics)
target_link_libraries(main generation)
target_link_libraries(main types)
