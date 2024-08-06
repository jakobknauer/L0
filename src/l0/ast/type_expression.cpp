#include "l0/ast/type_expression.h"

namespace l0
{

StructExpression::StructExpression(std::shared_ptr<StatementBlock> body) : body{body} {}

void StructExpression::Accept(IConstTypeExpressionVisitor& visitor) const { visitor.Visit(*this); }

}  // namespace l0
