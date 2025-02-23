add_library(compiler_driver compiler_driver.cpp compiler_driver.h)

target_link_libraries(
  compiler_driver
  LLVM
  lexing
  parsing
  semantics
  generation
  types)

add_executable(l0c main.cpp)

target_link_libraries(l0c compiler_driver)
