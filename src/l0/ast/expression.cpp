#include "l0/ast/expression.h"

#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"

namespace l0
{

Assignment::Assignment(std::unique_ptr<Expression> target, std::unique_ptr<Expression> expression)
    : target{std::move(target)}, expression{std::move(expression)}
{
}

void Assignment::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void Assignment::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

UnaryOp::UnaryOp(std::unique_ptr<Expression> operand, Operator op) : operand{std::move(operand)}, op{op} {}

void UnaryOp::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void UnaryOp::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

BinaryOp::BinaryOp(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, Operator op)
    : left{std::move(left)}, right{std::move(right)}, op{op}
{
}

void BinaryOp::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void BinaryOp::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

Variable::Variable(std::string name) : name{name} {}

void Variable::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void Variable::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

Call::Call(std::unique_ptr<Variable> function, std::unique_ptr<ArgumentList> arguments)
    : function{std::move(function)}, arguments{std::move(arguments)}
{
}

void Call::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void Call::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

void UnitLiteral::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void UnitLiteral::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

BooleanLiteral::BooleanLiteral(bool value) : value{value} {}

void BooleanLiteral::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void BooleanLiteral::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

IntegerLiteral::IntegerLiteral(std::int64_t value) : value{value} {}

void IntegerLiteral::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void IntegerLiteral::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

StringLiteral::StringLiteral(std::string value) : value{value} {}

void StringLiteral::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void StringLiteral::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

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

void Function::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void Function::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

Allocation::Allocation(std::unique_ptr<TypeAnnotation> annotation, std::unique_ptr<Expression> size)
    : annotation{std::move(annotation)}, size{std::move(size)}
{
}

void Allocation::Accept(IConstExpressionVisitor& visitor) const { visitor.Visit(*this); }

void Allocation::Accept(IExpressionVisitor& visitor) { visitor.Visit(*this); }

}  // namespace l0
