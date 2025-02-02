#ifndef L0_AST_TYPE_EXPRESSION_H
#define L0_AST_TYPE_EXPRESSION_H

#include <memory>
#include <vector>

namespace l0
{

class IConstTypeExpressionVisitor;
class ITypeExpressionVisitor;

class TypeExpression
{
   public:
    virtual ~TypeExpression() = default;

    virtual void Accept(IConstTypeExpressionVisitor& visitor) const = 0;
    virtual void Accept(ITypeExpressionVisitor& visitor) = 0;
};

class Declaration;
using StructMemberDeclarationList = std::vector<std::shared_ptr<Declaration>>;

class StructExpression : public TypeExpression
{
   public:
    StructExpression(std::shared_ptr<StructMemberDeclarationList> members);

    void Accept(IConstTypeExpressionVisitor& visitor) const override;
    void Accept(ITypeExpressionVisitor& visitor) override;

    std::shared_ptr<StructMemberDeclarationList> members;
};

struct EnumMemberDeclaration
{
    std::string name;
};
using EnumMemberDeclarationList = std::vector<std::shared_ptr<EnumMemberDeclaration>>;

class EnumExpression : public TypeExpression
{
   public:
    EnumExpression(std::shared_ptr<EnumMemberDeclarationList> members);

    void Accept(IConstTypeExpressionVisitor& visitor) const override;
    void Accept(ITypeExpressionVisitor& visitor) override;

    std::shared_ptr<EnumMemberDeclarationList> members;
};

class IConstTypeExpressionVisitor
{
   public:
    virtual ~IConstTypeExpressionVisitor() = default;

    virtual void Visit(const StructExpression& struct_expression) = 0;
    virtual void Visit([[maybe_unused]] const EnumExpression& enum_expression) {}
};

class ITypeExpressionVisitor
{
   public:
    virtual ~ITypeExpressionVisitor() = default;

    virtual void Visit(StructExpression& struct_expression) = 0;
    virtual void Visit([[maybe_unused]] EnumExpression& enum_expression) {}
};

};  // namespace l0

#endif
