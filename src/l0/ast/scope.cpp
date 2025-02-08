#include "l0/ast/scope.h"

namespace l0
{

void Scope::DeclareVariable(Identifier identifier)
{
    if (IsVariableDeclared(identifier))
    {
        throw ScopeError(std::format("Variable was '{}' declared before.", identifier.ToString()));
    }

    variables_.insert(identifier);
}

void Scope::DeclareVariable(Identifier identifier, std::shared_ptr<Type> type)
{
    DeclareVariable(identifier);
    SetVariableType(identifier, type);
}

bool Scope::IsVariableDeclared(Identifier identifier) const
{
    return variables_.contains(identifier);
}

void Scope::SetVariableType(Identifier identifier, std::shared_ptr<Type> type)
{
    if (!IsVariableDeclared(identifier))
    {
        throw ScopeError(std::format("Cannot set type of undeclared variable '{}'.", identifier.ToString()));
    }

    if (IsVariableTypeSet(identifier))
    {
        throw ScopeError(std::format("Type of variable '{}' was set before.", identifier.ToString()));
    }

    variable_types_.insert(std::make_pair(identifier, type));
}

bool Scope::IsVariableTypeSet(Identifier identifier) const
{
    return variable_types_.contains(identifier);
}

std::shared_ptr<Type> Scope::GetVariableType(Identifier identifier) const
{
    if (!IsVariableDeclared(identifier))
    {
        throw ScopeError(std::format("Cannot get type of undeclared variable '{}'.", identifier.ToString()));
    }

    if (!IsVariableTypeSet(identifier))
    {
        throw ScopeError(std::format("Type of variable '{}' is undefined.", identifier.ToString()));
    }

    return variable_types_.at(identifier);
}

void Scope::SetLLVMValue(Identifier identifier, llvm::Value* llvm_value)
{
    if (!IsVariableDeclared(identifier))
    {
        throw ScopeError(std::format("Cannot set LLVM Value of undeclared variable '{}'.", identifier.ToString()));
    }

    if (llvm_values_.contains(identifier))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' was set before.", identifier.ToString()));
    }

    llvm_values_.insert({identifier, llvm_value});
}

llvm::Value* Scope::GetLLVMValue(Identifier identifier) const
{
    if (!IsVariableDeclared(identifier))
    {
        throw ScopeError(std::format("Cannot get LLVM Value of undeclared variable '{}'.", identifier.ToString()));
    }

    if (!llvm_values_.contains(identifier))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' is undefined.", identifier.ToString()));
    }

    return llvm_values_.at(identifier);
}

void Scope::DeclareType(Identifier identifier)
{
    if (IsTypeDeclared(identifier))
    {
        throw ScopeError(std::format("Type '{}' was declared before.", identifier.ToString()));
    }

    types_.insert(identifier);
}

bool Scope::IsTypeDeclared(Identifier identifier) const
{
    return types_.contains(identifier);
}

void Scope::DefineType(Identifier identifier, std::shared_ptr<Type> type)
{
    if (!IsTypeDeclared(identifier))
    {
        throw ScopeError(std::format("Type '{}' is undefined.", identifier.ToString()));
    }

    if (IsTypeDefined(identifier))
    {
        throw ScopeError(std::format("Type '{}' was defined before.", identifier.ToString()));
    }

    type_definitions_.insert({identifier, type});
}

bool Scope::IsTypeDefined(Identifier identifier) const
{
    return type_definitions_.contains(identifier);
}

std::shared_ptr<Type> Scope::GetTypeDefinition(Identifier identifier) const
{
    if (!IsTypeDefined(identifier))
    {
        throw ScopeError(std::format("Type '{}' is undefined.", identifier.ToString()));
    }

    return type_definitions_.at(identifier);
}

void Scope::Clear()
{
    llvm_values_.clear();
    types_.clear();
    variables_.clear();
}

const std::unordered_set<Identifier>& Scope::GetVariables() const
{
    return variables_;
}

const std::unordered_set<Identifier>& Scope::GetTypes() const
{
    return types_;
}

void Scope::UpdateTypes(const Scope& other)
{
    for (const auto& [identifier, definition] : other.type_definitions_)
    {
        DeclareType(identifier);
        DefineType(identifier, definition);
    }
}

void Scope::UpdateVariables(const Scope& other)
{
    for (const auto& [identifier, type] : other.variable_types_)
    {
        DeclareVariable(identifier, type);
    }
}

ScopeError::ScopeError(const std::string& message)
    : message_{message}
{
}

std::string ScopeError::GetMessage() const
{
    return message_;
}

}  // namespace l0
