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

struct AddressInfo
{
    std::shared_ptr<Expression> object_ref{nullptr};
    std::shared_ptr<Type> object_type{nullptr};
    std::vector<std::size_t> member_indices{};
};

class Assignment : public Expression
{
   public:
    Assignment(std::shared_ptr<Expression> target, std::shared_ptr<Expression> expression);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<Expression> target;
    std::shared_ptr<Expression> expression;

    AddressInfo target_address;
};

class UnaryOp : public Expression
{
   public:
    enum class Operator
    {
        Plus,
        Minus,
        Bang,
        Ampersand,
        Asterisk,
    };

    UnaryOp(std::shared_ptr<Expression> operand, Operator op);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<Expression> operand;
    Operator op;
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

    BinaryOp(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, Operator op);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
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

class MemberAccessor : public Expression
{
   public:
    MemberAccessor(std::shared_ptr<Expression> object, std::string member);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<Expression> object;
    std::string member;

    mutable std::shared_ptr<StructType> object_type;
    mutable std::size_t member_index;
};

using ArgumentList = std::vector<std::shared_ptr<Expression>>;

class Call : public Expression
{
   public:
    Call(std::shared_ptr<Expression> function, std::shared_ptr<ArgumentList> arguments);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<Expression> function;
    std::shared_ptr<ArgumentList> arguments;
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
using StatementBlock = std::vector<std::shared_ptr<Statement>>;

class ParameterDeclaration
{
   public:
    ParameterDeclaration(std::string name, std::shared_ptr<TypeAnnotation> annotation);

    std::string name;
    std::shared_ptr<TypeAnnotation> annotation;
};

using ParameterDeclarationList = std::vector<std::shared_ptr<ParameterDeclaration>>;

class Function : public Expression
{
   public:
    Function(
        std::shared_ptr<ParameterDeclarationList> parameters,
        std::shared_ptr<TypeAnnotation> return_type_annotation,
        std::shared_ptr<StatementBlock> statements
    );

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<ParameterDeclarationList> parameters;
    std::shared_ptr<TypeAnnotation> return_type_annotation;
    std::shared_ptr<StatementBlock> statements;

    mutable std::shared_ptr<Scope> locals = std::make_shared<Scope>();
};

struct MemberInitializer
{
    std::string member;
    std::shared_ptr<Expression> value;
};
using MemberInitializerList = std::vector<std::shared_ptr<MemberInitializer>>;

class Initializer : public Expression
{
   public:
    Initializer(std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<MemberInitializerList> member_initializers);

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<TypeAnnotation> annotation;
    std::shared_ptr<MemberInitializerList> member_initializers;
};

class Allocation : public Expression
{
   public:
    Allocation(
        std::shared_ptr<TypeAnnotation> annotation,
        std::shared_ptr<Expression> size,
        std::shared_ptr<MemberInitializerList> member_initializers = nullptr
    );

    void Accept(IConstExpressionVisitor& visitor) const override;
    void Accept(IExpressionVisitor& visitor) override;

    std::shared_ptr<TypeAnnotation> annotation;
    std::shared_ptr<Expression> size;
    std::shared_ptr<MemberInitializerList> member_initializers;

    mutable std::shared_ptr<Type> allocated_type;
    mutable std::shared_ptr<Expression> initial_value;
};

class IConstExpressionVisitor
{
   public:
    virtual ~IConstExpressionVisitor() = default;

    virtual void Visit(const Assignment& assignment) = 0;
    virtual void Visit(const UnaryOp& unary_op) = 0;
    virtual void Visit(const BinaryOp& binary_op) = 0;
    virtual void Visit(const Variable& variable) = 0;
    virtual void Visit(const MemberAccessor& member_accessor) = 0;
    virtual void Visit(const Call& call) = 0;
    virtual void Visit(const UnitLiteral& literal) = 0;
    virtual void Visit(const BooleanLiteral& literal) = 0;
    virtual void Visit(const IntegerLiteral& literal) = 0;
    virtual void Visit(const StringLiteral& literal) = 0;
    virtual void Visit(const Function& function) = 0;
    virtual void Visit(const Initializer& initializer) = 0;
    virtual void Visit(const Allocation& allocation) = 0;
};

class IExpressionVisitor
{
   public:
    virtual ~IExpressionVisitor() = default;

    virtual void Visit(Assignment& assignment) = 0;
    virtual void Visit(UnaryOp& unary_op) = 0;
    virtual void Visit(BinaryOp& binary_op) = 0;
    virtual void Visit(Variable& variable) = 0;
    virtual void Visit(MemberAccessor& member_accessor) = 0;
    virtual void Visit(Call& call) = 0;
    virtual void Visit(UnitLiteral& literal) = 0;
    virtual void Visit(BooleanLiteral& literal) = 0;
    virtual void Visit(IntegerLiteral& literal) = 0;
    virtual void Visit(StringLiteral& literal) = 0;
    virtual void Visit(Function& function) = 0;
    virtual void Visit(Initializer& initializer) = 0;
    virtual void Visit(Allocation& allocation) = 0;
};

}  // namespace l0

#endif
