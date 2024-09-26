#include "l0/ast/statement.h"

namespace l0
{

StatementBlock::StatementBlock(std::vector<std::shared_ptr<Statement>> statements)
    : statements{statements}
{
}

void StatementBlock::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void StatementBlock::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

Declaration::Declaration(std::string variable, std::shared_ptr<Expression> initializer)
    : Declaration{variable, nullptr, initializer}
{
}

Declaration::Declaration(
    std::string variable, std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<Expression> initializer
)
    : variable{variable},
      annotation{annotation},
      initializer{initializer}
{
}

void Declaration::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Declaration::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

TypeDeclaration::TypeDeclaration(std::string name, std::shared_ptr<TypeExpression> definition)
    : name{name},
      definition{definition}
{
}

void TypeDeclaration::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void TypeDeclaration::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

ExpressionStatement::ExpressionStatement(std::shared_ptr<Expression> expression)
    : expression{expression}
{
}

void ExpressionStatement::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void ExpressionStatement::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

ReturnStatement::ReturnStatement(std::shared_ptr<Expression> value)
    : value{value}
{
}

void ReturnStatement::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void ReturnStatement::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

ConditionalStatement::ConditionalStatement(
    std::shared_ptr<Expression> condition,
    std::shared_ptr<StatementBlock> then_block,
    std::shared_ptr<StatementBlock> else_block
)
    : condition{condition},
      then_block{then_block},
      else_block{else_block}
{
}

void ConditionalStatement::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void ConditionalStatement::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

WhileLoop::WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<StatementBlock> body)
    : condition{condition},
      body{body}
{
}

void WhileLoop::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void WhileLoop::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

Deallocation::Deallocation(std::shared_ptr<Expression> reference)
    : reference{reference}
{
}

void Deallocation::Accept(IConstStatementVisitor& visitor) const
{
    visitor.Visit(*this);
}

void Deallocation::Accept(IStatementVisitor& visitor)
{
    visitor.Visit(*this);
}

}  // namespace l0
