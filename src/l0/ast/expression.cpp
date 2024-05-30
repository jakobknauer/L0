#include "l0/ast/expression.h"

#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"

namespace l0
{

Assignment::Assignment(std::string variable, std::unique_ptr<Expression> expression)
    : variable{variable}, expression{std::move(expression)}
{
}

void Assignment::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

BinaryOp::BinaryOp(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, Operator op)
    : left{std::move(left)}, right{std::move(right)}, op{op}
{
}

void BinaryOp::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

Variable::Variable(std::string name) : name{name} {}

void Variable::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

Call::Call(std::unique_ptr<Variable> function, std::unique_ptr<ArgumentList> arguments)
    : function{std::move(function)}, arguments{std::move(arguments)}
{
}

void Call::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

BooleanLiteral::BooleanLiteral(bool value) : value{value} {}

void BooleanLiteral::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

IntegerLiteral::IntegerLiteral(std::int64_t value) : value{value} {}

void IntegerLiteral::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

StringLiteral::StringLiteral(std::string value) : value{value} {}

void StringLiteral::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

ParameterDeclaration::ParameterDeclaration(std::string name, std::unique_ptr<TypeAnnotation> annotation)
    : name{name}, annotation{std::move(annotation)}
{
}

Function::Function(
    std::unique_ptr<ParameterDeclarationList> parameters,
    std::unique_ptr<TypeAnnotation> return_type_annotation,
    std::unique_ptr<StatementBlock> statements
)
    : parameters{std::move(parameters)},
      return_type_annotation{std::move(return_type_annotation)},
      statements{std::move(statements)}
{
}

void Function::Accept(IExpressionVisitor& visitor) const { visitor.Visit(*this); }

}  // namespace l0
