#ifndef L0_AST_TYPE_EXPRESSION_H
#define L0_AST_TYPE_EXPRESSION_H

#include <memory>
#include <vector>

namespace l0
{

class IConstTypeExpressionVisitor;

class TypeExpression
{
   public:
    virtual ~TypeExpression() = default;

    virtual void Accept(IConstTypeExpressionVisitor& visitor) const = 0;
};

class Statement;
using StatementBlock = std::vector<std::shared_ptr<Statement>>;

class StructExpression : public TypeExpression
{
   public:
    StructExpression(std::shared_ptr<StatementBlock> body);

    void Accept(IConstTypeExpressionVisitor& visitor) const override;

    std::shared_ptr<StatementBlock> body;
};

class IConstTypeExpressionVisitor
{
   public:
    virtual ~IConstTypeExpressionVisitor() = default;

    virtual void Visit(const StructExpression& struct_expression) = 0;
};

};  // namespace l0

#endif
