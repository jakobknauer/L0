#include "l0/semantics/resolver.h"

#include <ranges>

#include "l0/semantics/semantic_error.h"

namespace l0
{

Resolver::Resolver(const Module& module)
    : module_{module}
{
}

void Resolver::Check()
{
    scopes_.clear();
    scopes_.push_back(module_.environment);
    scopes_.push_back(module_.externals);
    scopes_.push_back(module_.globals);

    namespaces_.push(Identifier{});

    for (auto callable : module_.callables)
    {
        callable->Accept(*this);
    }
}

void Resolver::Visit(const StatementBlock& statement_block)
{
    for (auto statement : statement_block.statements)
    {
        statement->Accept(*this);
    }
}

void Resolver::Visit(const Declaration& declaration)
{
    if (declaration.initializer)
    {
        declaration.initializer->Accept(*this);
    }

    auto scope = scopes_.back();
    if (scope->IsVariableDeclared(declaration.identifier))
    {
        throw SemanticError(
            std::format("Duplicate declaration of local variable '{}'.", declaration.identifier.ToString())
        );
    }
    scope->DeclareVariable(declaration.identifier);
    declaration.scope = scope;
}

void Resolver::Visit(const TypeDeclaration& type_declaration)
{
    type_declaration.definition->Accept(*this);
}

void Resolver::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
}

void Resolver::Visit(const ReturnStatement& return_statement)
{
    return_statement.value->Accept(*this);
}

void Resolver::Visit(const ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);

    scopes_.push_back(std::make_shared<Scope>());
    conditional_statement.then_block->Accept(*this);
    scopes_.pop_back();

    if (!conditional_statement.else_block)
    {
        return;
    }

    scopes_.push_back(std::make_shared<Scope>());
    conditional_statement.else_block->Accept(*this);
    scopes_.pop_back();
}

void Resolver::Visit(const WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);

    scopes_.push_back(std::make_shared<Scope>());
    while_loop.body->Accept(*this);
    scopes_.pop_back();
}

void Resolver::Visit(const Deallocation& deallocation)
{
    deallocation.reference->Accept(*this);
}

void Resolver::Visit(const Assignment& assignment)
{
    assignment.target->Accept(*this);
    assignment.expression->Accept(*this);
}

void Resolver::Visit(const UnaryOp& unary_op)
{
    unary_op.operand->Accept(*this);
}

void Resolver::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    binary_op.right->Accept(*this);
}

void Resolver::Visit(const Variable& variable)
{
    const auto [scope, resolved_name] = Resolve(variable.name, namespaces_.top());
    variable.scope = scope;
    variable.resolved_name = resolved_name;
}

void Resolver::Visit(const MemberAccessor& member_accessor)
{
    member_accessor.object->Accept(*this);
}

void Resolver::Visit(const Call& call)
{
    call.function->Accept(*this);
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
    }
}

void Resolver::Visit(const UnitLiteral&) {}
void Resolver::Visit(const BooleanLiteral&) {}
void Resolver::Visit(const IntegerLiteral&) {}
void Resolver::Visit(const CharacterLiteral&) {}
void Resolver::Visit(const StringLiteral&) {}

void Resolver::Visit(const Function& function)
{
    if (function.captures)
    {
        for (const auto& capture : *function.captures)
        {
            capture->Accept(*this);
            function.locals->DeclareVariable(capture->name);
        }
    }

    auto scopes_backup = scopes_;

    scopes_.clear();
    scopes_.push_back(module_.environment);
    scopes_.push_back(module_.externals);
    scopes_.push_back(module_.globals);
    scopes_.push_back(function.locals);

    for (const auto& param_decl : *function.parameters)
    {
        function.locals->DeclareVariable(param_decl->name);
    }

    namespaces_.push(function.namespace_);
    function.body->Accept(*this);
    namespaces_.pop();

    scopes_ = scopes_backup;
}

void Resolver::Visit(const Initializer& initializer)
{
    for (const auto& member_initializer : *initializer.member_initializers)
    {
        member_initializer->value->Accept(*this);
    }
}

void Resolver::Visit(const Allocation& allocation)
{
    if (allocation.size)
    {
        allocation.size->Accept(*this);
    }
    if (allocation.member_initializers)
    {
        for (const auto& member_initializer : *allocation.member_initializers)
        {
            member_initializer->value->Accept(*this);
        }
    }
}

void Resolver::Visit(const StructExpression&)
{
    throw SemanticError("Obsolete");
}

void Resolver::Visit(const EnumExpression&)
{
    throw SemanticError("Obsolete");
}

std::optional<std::shared_ptr<Scope>> Resolver::Resolve(const Identifier& identifier)
{
    for (auto scope : scopes_ | std::views::reverse)
    {
        if (scope->IsVariableDeclared(identifier))
        {
            return scope;
        }
    }
    return std::nullopt;
}

std::pair<std::shared_ptr<Scope>, Identifier> Resolver::Resolve(
    const Identifier& identifier, const Identifier& namespace_
)
{
    auto global_resolution = Resolve(identifier);
    if (global_resolution)
    {
        return {*global_resolution, identifier};
    }

    auto resolution_in_namespace = Resolve(namespace_ + identifier);
    if (resolution_in_namespace)
    {
        return {*resolution_in_namespace, namespace_ + identifier};
    }

    throw SemanticError(std::format("Cannot resolve variable '{}'.", identifier.ToString()));
}

}  // namespace l0
