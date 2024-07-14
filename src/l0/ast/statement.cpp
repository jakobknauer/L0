#include "l0/ast/statement.h"

namespace l0
{

Declaration::Declaration(
    std::string variable, std::unique_ptr<TypeAnnotation> annotation, std::unique_ptr<Expression> initializer
)
    : variable{variable}, annotation{std::move(annotation)}, initializer{std::move(initializer)}
{
}

void Declaration::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void Declaration::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression> expression) : expression{std::move(expression)} {}

void ExpressionStatement::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void ExpressionStatement::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

ReturnStatement::ReturnStatement(std::unique_ptr<Expression> value) : value{std::move(value)} {}

void ReturnStatement::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void ReturnStatement::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

ConditionalStatement::ConditionalStatement(
    std::unique_ptr<Expression> condition,
    std::unique_ptr<StatementBlock> if_block,
    std::unique_ptr<StatementBlock> else_block
)
    : condition{std::move(condition)}, then_block{std::move(if_block)}, else_block{std::move(else_block)}
{
}

void ConditionalStatement::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void ConditionalStatement::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

WhileLoop::WhileLoop(std::unique_ptr<Expression> condition, std::unique_ptr<StatementBlock> body)
    : condition{std::move(condition)}, body{std::move(body)}
{
}

void WhileLoop::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void WhileLoop::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

Deallocation::Deallocation(std::unique_ptr<Expression> reference) : reference{std::move(reference)} {}

void Deallocation::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void Deallocation::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

}  // namespace l0
