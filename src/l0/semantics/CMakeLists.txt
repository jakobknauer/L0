add_library(
  semantics
  binary_op_overload_resolver.cpp
  binary_op_overload_resolver.h
  global_symbol_pass.cpp
  global_symbol_pass.h
  module_validator.cpp
  module_validator.h
  resolver.cpp
  resolver.h
  return_statement_pass.cpp
  return_statement_pass.h
  semantic_error.cpp
  semantic_error.h
  type_annotation_converter.cpp
  type_annotation_converter.h
  typechecker.cpp
  typechecker.h)

target_link_libraries(semantics ast types)
