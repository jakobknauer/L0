#include "l0/ast/type_expression.h"

namespace l0
{

StructExpression::StructExpression(std::shared_ptr<StructMemberDeclarationList> members)
    : members{members}
{
}

void StructExpression::Accept(IConstTypeExpressionVisitor& visitor) const
{
    visitor.Visit(*this);
}

void StructExpression::Accept(ITypeExpressionVisitor& visitor)
{
    visitor.Visit(*this);
}

}  // namespace l0
