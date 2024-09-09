add_library(
  ast
  ast_printer.cpp
  ast_printer.h
  expression.cpp
  expression.h
  indent.cpp
  indent.h
  module.h
  scope.cpp
  scope.h
  statement.cpp
  statement.h
  type_annotation.cpp
  type_annotation.h
  type_expression.cpp
  type_expression.h)

target_link_libraries(ast types)
