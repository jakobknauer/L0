#include "l0/semantics/return_statement_pass.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

ReturnStatementPass::ReturnStatementPass(Module& module) : module_{module} {}

void ReturnStatementPass::Run() { Visit(*module_.statements); }

void ReturnStatementPass::Visit(Declaration& declaration)
{
    declaration.initializer->Accept(*this);
    statement_returns_ = false;
}

void ReturnStatementPass::Visit(ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
    statement_returns_ = false;
}

void ReturnStatementPass::Visit(ReturnStatement& return_statement)
{
    if (!conversion_checker_.CheckCompatibility(expected_return_value_.top(), return_statement.value->type))
    {
        throw SemanticError(std::format(
            "Expected return value of type {}, but got incompatible type {} instead.",
            expected_return_value_.top()->ToString(),
            return_statement.value->type->ToString()
        ));
    }

    return_statement.value->Accept(*this);

    statement_returns_ = true;
}

void ReturnStatementPass::Visit(ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);

    Visit(*conditional_statement.then_block);
    conditional_statement.then_block_returns = statement_returns_;

    if (conditional_statement.else_block)
    {
        Visit(*conditional_statement.else_block);
        conditional_statement.else_block_returns = statement_returns_;
    }

    statement_returns_ = conditional_statement.then_block_returns && conditional_statement.else_block_returns;
}

void ReturnStatementPass::Visit(WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);
    Visit(*while_loop.body);
    statement_returns_ = false;
}

void ReturnStatementPass::Visit(Deallocation& deallocation)
{
    deallocation.reference->Accept(*this);
    statement_returns_ = false;
}

void ReturnStatementPass::Visit(Assignment& assignment)
{
    assignment.target->Accept(*this);
    assignment.expression->Accept(*this);
}

void ReturnStatementPass::Visit(UnaryOp& unary_op) { unary_op.operand->Accept(*this); }

void ReturnStatementPass::Visit(BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    binary_op.right->Accept(*this);
}

void ReturnStatementPass::Visit(Variable& variable) {}

void ReturnStatementPass::Visit(Call& call)
{
    call.function->Accept(*this);
    for (const auto& argument : *call.arguments)
    {
        argument->Accept(*this);
    }
}

void ReturnStatementPass::Visit(UnitLiteral& literal) {}
void ReturnStatementPass::Visit(BooleanLiteral& literal) {}
void ReturnStatementPass::Visit(IntegerLiteral& literal) {}
void ReturnStatementPass::Visit(StringLiteral& literal) {}

void ReturnStatementPass::Visit(Function& function)
{
    auto function_type = std::dynamic_pointer_cast<FunctionType>(function.type);
    if (!function_type)
    {
        throw SemanticError("Type of function must be function type.");
    }

    expected_return_value_.push(function_type->return_type);
    Visit(*function.statements);
    expected_return_value_.pop();

    if (!statement_returns_)
    {
        if (*function_type->return_type == UnitType{TypeQualifier::Constant})
        {
            auto return_statement = std::make_shared<ReturnStatement>(std::make_shared<UnitLiteral>());
            return_statement->value->type = std::make_shared<UnitType>(TypeQualifier::Constant);
            function.statements->push_back(return_statement);
        }
        else
        {
            throw SemanticError("Not all branches of function return a value.");
        }
    }

    statement_returns_ = false;
}

void ReturnStatementPass::Visit(Allocation& allocation)
{
    if (allocation.size)
    {
        allocation.size->Accept(*this);
    }
}

void ReturnStatementPass::Visit(StatementBlock& statement_block)
{
    bool block_returns_{false};
    auto first_return = statement_block.end();
    for (auto it = statement_block.begin(); it != statement_block.end(); ++it)
    {
        auto& statement = *it;
        statement->Accept(*this);

        if (statement_returns_ && !block_returns_)
        {
            block_returns_ = true;
            first_return = it;
        }
    }

    if (block_returns_)
    {
        statement_block.erase(first_return + 1, statement_block.end());
    }

    statement_returns_ = block_returns_;
}

}  // namespace l0
