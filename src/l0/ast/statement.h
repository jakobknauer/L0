#ifndef L0_AST_STATEMENT_H
#define L0_AST_STATEMENT_H

#include <memory>
#include <vector>

#include "l0/ast/expression.h"
#include "l0/ast/type_annotation.h"
#include "l0/ast/type_expression.h"

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

class StatementBlock : public Statement
{
   public:
    StatementBlock(std::vector<std::shared_ptr<Statement>> statements);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::vector<std::shared_ptr<Statement>> statements;
};

class Declaration : public Statement
{
   public:
    Declaration(std::string variable, std::shared_ptr<Expression> initializer);
    Declaration(
        std::string variable, std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<Expression> initializer
    );

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::string variable;
    std::shared_ptr<TypeAnnotation> annotation;
    std::shared_ptr<Expression> initializer;

    mutable std::shared_ptr<Scope> scope;
};

class TypeDeclaration : public Statement
{
   public:
    TypeDeclaration(std::string name, std::shared_ptr<TypeExpression> definition);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::string name;
    std::shared_ptr<TypeExpression> definition;

    mutable std::shared_ptr<Type> type;
};

class ExpressionStatement : public Statement
{
   public:
    ExpressionStatement(std::shared_ptr<Expression> expression);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::shared_ptr<Expression> expression;
};

class ReturnStatement : public Statement
{
   public:
    ReturnStatement(std::shared_ptr<Expression> value);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::shared_ptr<Expression> value;
};

class ConditionalStatement : public Statement
{
   public:
    ConditionalStatement(
        std::shared_ptr<Expression> condition,
        std::shared_ptr<StatementBlock> then_block,
        std::shared_ptr<StatementBlock> else_block = nullptr
    );

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::shared_ptr<Expression> condition;
    std::shared_ptr<StatementBlock> then_block;
    std::shared_ptr<StatementBlock> else_block;

    bool then_block_returns{false};
    bool else_block_returns{false};
};

class WhileLoop : public Statement
{
   public:
    WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> body);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::shared_ptr<Expression> condition;
    std::shared_ptr<StatementBlock> body;
};

class Deallocation : public Statement
{
   public:
    enum class DeallocationType
    {
        None,
        Reference,
        Closure,
    };

    Deallocation(std::shared_ptr<Expression> reference);

    void Accept(IConstStatementVisitor& visitor) const override;
    void Accept(IStatementVisitor& visitor) override;

    std::shared_ptr<Expression> reference;
    mutable DeallocationType deallocation_type{DeallocationType::None};
};

class IConstStatementVisitor
{
   public:
    virtual ~IConstStatementVisitor() = default;

    virtual void Visit(const StatementBlock& statement_block) = 0;
    virtual void Visit(const Declaration& declaration) = 0;
    virtual void Visit(const TypeDeclaration& type_declaration) = 0;
    virtual void Visit(const ExpressionStatement& expression_statement) = 0;
    virtual void Visit(const ReturnStatement& return_statement) = 0;
    virtual void Visit(const ConditionalStatement& conditional_statement) = 0;
    virtual void Visit(const WhileLoop& while_loop) = 0;
    virtual void Visit(const Deallocation& deallocation) = 0;
};

class IStatementVisitor
{
   public:
    virtual ~IStatementVisitor() = default;

    virtual void Visit(StatementBlock& statement_block) = 0;
    virtual void Visit(Declaration& declaration) = 0;
    virtual void Visit(TypeDeclaration& type_declaration) = 0;
    virtual void Visit(ExpressionStatement& expression_statement) = 0;
    virtual void Visit(ReturnStatement& return_statement) = 0;
    virtual void Visit(ConditionalStatement& conditional_statement) = 0;
    virtual void Visit(WhileLoop& while_loop) = 0;
    virtual void Visit(Deallocation& deallocation) = 0;
};

}  // namespace l0

#endif
