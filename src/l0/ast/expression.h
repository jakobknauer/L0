#ifndef L0_AST_EXPRESSION_H
#define L0_AST_EXPRESSION_H

#include <memory>
#include <vector>

#include "l0/ast/scope.h"
#include "l0/types/types.h"

namespace l0
{

class IConstExpressionVisitor;
class IExpressionVisitor;

class Expression
{
   public:
    virtual ~Expression() = default;
    virtual void Accept(IConstExpressionVisitor& visitor) const = 0;
    virtual void Accept(IExpressionVisitor& visitor) = 0;

    mutable std::shared_ptr<Type> type;
};

class Assignment : public Expression
{
   public:
    Assignment(std::string variable, std::unique_ptr<Expression> expression);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::string variable;
    std::unique_ptr<Expression> expression;

    mutable std::shared_ptr<Scope> scope;
};

class BinaryOp : public Expression
{
   public:
    enum class Operator
    {
        Plus,
        Minus,
        Asterisk,
        AmpersandAmpersand,
        PipePipe,
        EqualsEquals,
        BangEquals,
    };

    BinaryOp(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, Operator op);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    Operator op;
};

class Variable : public Expression
{
   public:
    Variable(std::string name);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::string name;

    mutable std::shared_ptr<Scope> scope;
};

using ArgumentList = std::vector<std::unique_ptr<Expression>>;

class Call : public Expression
{
   public:
    Call(std::unique_ptr<Variable> function, std::unique_ptr<ArgumentList> arguments);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::unique_ptr<Variable> function;
    std::unique_ptr<ArgumentList> arguments;
};

class UnitLiteral : public Expression
{
   public:
    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;
};

class BooleanLiteral : public Expression
{
   public:
    BooleanLiteral(bool value);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    bool value;
};

class IntegerLiteral : public Expression
{
   public:
    IntegerLiteral(std::int64_t value);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::int64_t value;
};

class StringLiteral : public Expression
{
   public:
    StringLiteral(std::string value);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::string value;
};

class TypeAnnotation;
class Statement;
using StatementBlock = std::vector<std::unique_ptr<Statement>>;

class ParameterDeclaration
{
   public:
    ParameterDeclaration(std::string name, std::unique_ptr<TypeAnnotation> annotation);

    std::string name;
    std::unique_ptr<TypeAnnotation> annotation;

    mutable std::shared_ptr<Type> type;
};

using ParameterDeclarationList = std::vector<std::unique_ptr<ParameterDeclaration>>;

class Function : public Expression
{
   public:
    Function(
        std::unique_ptr<ParameterDeclarationList> parameters,
        std::unique_ptr<TypeAnnotation> return_type_annotation,
        std::unique_ptr<StatementBlock> statements
    );

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::unique_ptr<ParameterDeclarationList> parameters;
    std::unique_ptr<TypeAnnotation> return_type_annotation;
    std::unique_ptr<StatementBlock> statements;

    mutable std::shared_ptr<Type> return_type;
    mutable std::shared_ptr<Scope> locals = std::make_shared<Scope>();
};

class IConstExpressionVisitor
{
   public:
    virtual ~IConstExpressionVisitor() = default;

    virtual void Visit(const Assignment& assignment) = 0;
    virtual void Visit(const BinaryOp& binary_op) = 0;
    virtual void Visit(const Variable& variable) = 0;
    virtual void Visit(const Call& call) = 0;
    virtual void Visit(const UnitLiteral& literal) = 0;
    virtual void Visit(const BooleanLiteral& literal) = 0;
    virtual void Visit(const IntegerLiteral& literal) = 0;
    virtual void Visit(const StringLiteral& literal) = 0;
    virtual void Visit(const Function& function) = 0;
};

class IExpressionVisitor
{
   public:
    virtual ~IExpressionVisitor() = default;

    virtual void Visit(Assignment& assignment) = 0;
    virtual void Visit(BinaryOp& binary_op) = 0;
    virtual void Visit(Variable& variable) = 0;
    virtual void Visit(Call& call) = 0;
    virtual void Visit(UnitLiteral& literal) = 0;
    virtual void Visit(BooleanLiteral& literal) = 0;
    virtual void Visit(IntegerLiteral& literal) = 0;
    virtual void Visit(StringLiteral& literal) = 0;
    virtual void Visit(Function& function) = 0;
};

}  // namespace l0

#endif
