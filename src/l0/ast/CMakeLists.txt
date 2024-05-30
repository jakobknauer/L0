add_library(
  ast
  ast_printer.h
  ast_printer.cpp
  expression.h
  expression.cpp
  module.h
  scope.h
  scope.cpp
  statement.h
  statement.cpp
  type_annotation.h
  type_annotation.cpp)

target_link_libraries(ast types)
