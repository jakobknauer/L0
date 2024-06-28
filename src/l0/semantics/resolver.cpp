#include "l0/semantics/resolver.h"

#include <ranges>

#include "l0/semantics/semantic_error.h"

namespace l0
{

Resolver::Resolver(const Module& module) : module_{module} {}

void Resolver::Check()
{
    scopes_.clear();
    scopes_.push_back(module_.externals);
    scopes_.push_back(module_.globals);
    for (auto& statement : *module_.statements)
    {
        statement->Accept(*this);
    }
}

void Resolver::Visit(const Declaration& declaration)
{
    declaration.initializer->Accept(*this);
    if (local_)
    {
        auto scope = scopes_.back();
        if (scope->IsDeclared(declaration.variable))
        {
            throw SemanticError(std::format("Duplicate declaration of local variable '{}'.", declaration.variable));
        }
        auto type = converter_.Convert(*declaration.annotation);
        scopes_.back()->Declare(declaration.variable, type);
        declaration.scope = scope;
    }
}

void Resolver::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
}

void Resolver::Visit(const ReturnStatement& return_statement) { return_statement.value->Accept(*this); }

void Resolver::Visit(const ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);

    scopes_.push_back(std::make_shared<Scope>());
    for (auto& statement : *conditional_statement.then_block)
    {
        statement->Accept(*this);
    }
    scopes_.pop_back();

    if (!conditional_statement.else_block)
    {
        return;
    }

    scopes_.push_back(std::make_shared<Scope>());
    for (auto& statement : *conditional_statement.else_block)
    {
        statement->Accept(*this);
    }
    scopes_.pop_back();
}

void Resolver::Visit(const WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);

    scopes_.push_back(std::make_shared<Scope>());
    for (auto& statement : *while_loop.body)
    {
        statement->Accept(*this);
    }
    scopes_.pop_back();
}

void Resolver::Visit(const Assignment& assignment)
{
    assignment.scope = Resolve(assignment.variable);
    assignment.expression->Accept(*this);
}

void Resolver::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    binary_op.right->Accept(*this);
}

void Resolver::Visit(const Variable& variable) { variable.scope = Resolve(variable.name); }

void Resolver::Visit(const Call& call)
{
    call.function->Accept(*this);
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
    }
}

void Resolver::Visit(const UnitLiteral& literal) {}

void Resolver::Visit(const BooleanLiteral& literal) {}

void Resolver::Visit(const IntegerLiteral& literal) {}

void Resolver::Visit(const StringLiteral& literal) {}

void Resolver::Visit(const Function& function)
{
    scopes_.push_back(function.locals);
    bool restore_local = local_;
    local_ = true;

    for (const auto& param_decl : *function.parameters)
    {
        auto type = converter_.Convert(*param_decl->annotation);
        param_decl->type = type;
        scopes_.back()->Declare(param_decl->name, type);
    }

    for (const auto& statement : *function.statements)
    {
        statement->Accept(*this);
    }

    function.return_type = converter_.Convert(*function.return_type_annotation);

    local_ = restore_local;
    scopes_.pop_back();
}

std::shared_ptr<Scope> Resolver::Resolve(const std::string name)
{
    for (auto scope : scopes_ | std::views::reverse)
    {
        if (scope->IsDeclared(name))
        {
            return scope;
        }
    }
    throw SemanticError(std::format("Usage of undeclared variable: '{}'.", name));
}

}  // namespace l0
