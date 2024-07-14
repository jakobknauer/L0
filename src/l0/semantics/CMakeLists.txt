add_library(
  semantics
  global_symbol_pass.cpp
  global_symbol_pass.h
  module_validator.cpp
  module_validator.h
  operator_overload_resolver.cpp
  operator_overload_resolver.h
  reference_pass.cpp
  reference_pass.h
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
