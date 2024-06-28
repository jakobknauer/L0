#ifndef L0_AST_STATEMENT_H
#define L0_AST_STATEMENT_H

#include <memory>
#include <vector>

#include "l0/ast/expression.h"
#include "l0/ast/type_annotation.h"

namespace l0
{

class IConstStatementVisitor;
class IStatementVisitor;

class Statement
{
   public:
    virtual ~Statement() = default;

    virtual void Accept(IConstStatementVisitor& visitor) const = 0;
    virtual void Accept(IStatementVisitor& visitor) = 0;
};

using StatementBlock = std::vector<std::unique_ptr<Statement>>;

class Declaration : public Statement
{
   public:
    Declaration(
        std::string variable, std::unique_ptr<TypeAnnotation> annotation, std::unique_ptr<Expression> initializer
    );

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::string variable;
    std::unique_ptr<TypeAnnotation> annotation;
    std::unique_ptr<Expression> initializer;

    mutable std::shared_ptr<Scope> scope;
};

class ExpressionStatement : public Statement
{
   public:
    ExpressionStatement(std::unique_ptr<Expression> expression);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::unique_ptr<Expression> expression;
};

class ReturnStatement : public Statement
{
   public:
    ReturnStatement(std::unique_ptr<Expression> value);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::unique_ptr<Expression> value;
};

class ConditionalStatement : public Statement
{
   public:
    ConditionalStatement(
        std::unique_ptr<Expression> condition,
        std::unique_ptr<StatementBlock> then_block,
        std::unique_ptr<StatementBlock> else_block = nullptr
    );

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::unique_ptr<Expression> condition;
    std::unique_ptr<StatementBlock> then_block;
    std::unique_ptr<StatementBlock> else_block;
};

class WhileLoop : public Statement
{
   public:
    WhileLoop(std::unique_ptr<Expression> condition, std::unique_ptr<StatementBlock> body);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::unique_ptr<Expression> condition;
    std::unique_ptr<StatementBlock> body;
};

class IConstStatementVisitor
{
   public:
    virtual ~IConstStatementVisitor() = default;

    virtual void Visit(const Declaration& declaration) = 0;
    virtual void Visit(const ExpressionStatement& expression_statement) = 0;
    virtual void Visit(const ReturnStatement& return_statement) = 0;
    virtual void Visit(const ConditionalStatement& conditional_statement) = 0;
    virtual void Visit(const WhileLoop& while_loop) = 0;
};

class IStatementVisitor
{
   public:
    virtual ~IStatementVisitor() = default;

    virtual void Visit(Declaration& declaration) = 0;
    virtual void Visit(ExpressionStatement& expression_statement) = 0;
    virtual void Visit(ReturnStatement& return_statement) = 0;
    virtual void Visit(ConditionalStatement& conditional_statement) = 0;
    virtual void Visit(WhileLoop& while_loop) = 0;
};

}  // namespace l0

#endif
