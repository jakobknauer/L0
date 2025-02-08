#include "l0/ast/expression.h"

#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"

namespace l0
{

Assignment::Assignment(std::shared_ptr<Expression> target, std::shared_ptr<Expression> expression)
    : target{target},
      expression{expression}
{
}

void Assignment::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Assignment::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

UnaryOp::UnaryOp(std::shared_ptr<Expression> operand, Operator op)
    : operand{operand},
      op{op}
{
}

void UnaryOp::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void UnaryOp::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

BinaryOp::BinaryOp(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, Operator op)
    : left{left},
      right{right},
      op{op}
{
}

void BinaryOp::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void BinaryOp::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

Variable::Variable(Identifier name)
    : name{name}
{
}

void Variable::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Variable::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

MemberAccessor::MemberAccessor(std::shared_ptr<Expression> object, std::string member)
    : object{object},
      member{member}
{
}

void MemberAccessor::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void MemberAccessor::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

Call::Call(std::shared_ptr<Expression> function, std::shared_ptr<ArgumentList> arguments)
    : function{function},
      arguments{arguments}
{
}

void Call::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Call::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

void UnitLiteral::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void UnitLiteral::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

BooleanLiteral::BooleanLiteral(bool value)
    : value{value}
{
}

void BooleanLiteral::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void BooleanLiteral::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

IntegerLiteral::IntegerLiteral(std::int64_t value)
    : value{value}
{
}

void IntegerLiteral::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void IntegerLiteral::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

CharacterLiteral::CharacterLiteral(char8_t value)
    : value{value}
{
}

void CharacterLiteral::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void CharacterLiteral::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

StringLiteral::StringLiteral(std::string value)
    : value{value}
{
}

void StringLiteral::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void StringLiteral::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

ParameterDeclaration::ParameterDeclaration(std::string name, std::shared_ptr<TypeAnnotation> annotation)
    : name{name},
      annotation{annotation}
{
}

Function::Function(
    std::shared_ptr<ParameterDeclarationList> parameters,
    std::shared_ptr<CaptureList> captures,
    std::shared_ptr<TypeAnnotation> return_type_annotation,
    std::shared_ptr<StatementBlock> body,
    Identifier namespace_
)
    : parameters{parameters},
      captures{captures},
      return_type_annotation{return_type_annotation},
      body{body},
      namespace_{namespace_}
{
}

void Function::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Function::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

Initializer::Initializer(
    std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<MemberInitializerList> member_initializers
)
    : annotation{annotation},
      member_initializers{member_initializers}
{
}

void Initializer::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Initializer::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

Allocation::Allocation(
    std::shared_ptr<TypeAnnotation> annotation,
    std::shared_ptr<Expression> size,
    std::shared_ptr<MemberInitializerList> member_initializers
)
    : annotation{annotation},
      size{size},
      member_initializers{member_initializers}
{
}

void Allocation::Accept(IConstExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Allocation::Accept(IExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

}  // namespace l0
