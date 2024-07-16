#include "l0/ast/statement.h"

namespace l0
{

Declaration::Declaration(
    std::string variable, std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<Expression> initializer
)
    : variable{variable}, annotation{annotation}, initializer{initializer}
{
}

void Declaration::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void Declaration::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

ExpressionStatement::ExpressionStatement(std::shared_ptr<Expression> expression) : expression{expression} {}

void ExpressionStatement::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void ExpressionStatement::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

ReturnStatement::ReturnStatement(std::shared_ptr<Expression> value) : value{value} {}

void ReturnStatement::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void ReturnStatement::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

ConditionalStatement::ConditionalStatement(
    std::shared_ptr<Expression> condition,
    std::shared_ptr<StatementBlock> if_block,
    std::shared_ptr<StatementBlock> else_block
)
    : condition{condition}, then_block{if_block}, else_block{else_block}
{
}

void ConditionalStatement::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void ConditionalStatement::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

WhileLoop::WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> body)
    : condition{condition}, body{body}
{
}

void WhileLoop::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void WhileLoop::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

Deallocation::Deallocation(std::shared_ptr<Expression> reference) : reference{reference} {}

void Deallocation::Accept(IConstStatementVisitor& visitor) const { visitor.Visit(*this); }

void Deallocation::Accept(IStatementVisitor& visitor) { visitor.Visit(*this); }

}  // namespace l0
