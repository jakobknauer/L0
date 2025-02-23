add_library(
  semantics
  conversion_checker.cpp
  conversion_checker.h
  declare_global_types.cpp
  declare_global_types.h
  declare_variables.cpp
  declare_variables.h
  fill_types.cpp
  fill_types.h
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
  semantics.cpp
  semantics.h
  type_resolver.cpp
  type_resolver.h
  typechecker.cpp
  typechecker.h)

target_link_libraries(semantics ast types common)
