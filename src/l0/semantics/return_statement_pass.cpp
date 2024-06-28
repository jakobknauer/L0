#include "l0/semantics/return_statement_pass.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

ReturnStatementPass::ReturnStatementPass(Module& module) : module_{module} {}

void ReturnStatementPass::Run() { Visit(*module_.statements); }

void ReturnStatementPass::Visit(const Declaration& declaration)
{
    declaration.initializer->Accept(*this);
    always_returns_ = false;
}

void ReturnStatementPass::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
    always_returns_ = false;
}

void ReturnStatementPass::Visit(const ReturnStatement& return_statement)
{
    if (*return_statement.value->type != *expected_return_value_.top())
    {
        throw SemanticError(std::format(
            "Expected return value of type {}, but got {} instead.",
            expected_return_value_.top()->ToString(),
            return_statement.value->type->ToString()
        ));
    }

    return_statement.value->Accept(*this);

    always_returns_ = true;
}

void ReturnStatementPass::Visit(const ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);

    Visit(*conditional_statement.then_block);
    bool then_returns{always_returns_};

    bool else_returns{false};
    if (conditional_statement.else_block)
    {
        Visit(*conditional_statement.else_block);
        else_returns = always_returns_;
    }

    always_returns_ = then_returns && else_returns;
}

void ReturnStatementPass::Visit(const WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);
    Visit(*while_loop.body);
    always_returns_ = false;
}

void ReturnStatementPass::Visit(const Assignment& assignment) { assignment.expression->Accept(*this); }

void ReturnStatementPass::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    binary_op.right->Accept(*this);
}

void ReturnStatementPass::Visit(const Variable& variable) {}

void ReturnStatementPass::Visit(const Call& call)
{
    call.function->Accept(*this);
    for (const auto& argument : *call.arguments)
    {
        argument->Accept(*this);
    }
}

void ReturnStatementPass::Visit(const UnitLiteral& literal) {}
void ReturnStatementPass::Visit(const BooleanLiteral& literal) {}
void ReturnStatementPass::Visit(const IntegerLiteral& literal) {}
void ReturnStatementPass::Visit(const StringLiteral& literal) {}

void ReturnStatementPass::Visit(const Function& function)
{
    expected_return_value_.push(function.return_type.get());
    Visit(*function.statements);
    expected_return_value_.pop();

    if (!always_returns_)
    {
        throw SemanticError("Not all branches of function return a value.");
    }

    always_returns_ = false;
}

void ReturnStatementPass::Visit(const StatementBlock& statement_block)
{
    bool returns{false};
    for (const auto& statement : statement_block)
    {
        statement->Accept(*this);
        returns |= always_returns_;
    }
    always_returns_ = returns;
}

}  // namespace l0
