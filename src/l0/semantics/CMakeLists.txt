add_library(
  semantics
  conversion_checker.cpp
  conversion_checker.h
  global_scope_builder.cpp
  global_scope_builder.h
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
  type_resolver.cpp
  type_resolver.h
  typechecker.cpp
  typechecker.h)

target_link_libraries(semantics ast types common)
