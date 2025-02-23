add_library(
  generation
  generation.cpp
  generation.h
  generator.cpp
  generator.h
  generator_error.cpp
  generator_error.h
  type_converter.cpp
  type_converter.h)

target_link_libraries(generation common)
