#include "l0/ast/scope.h"

namespace l0
{

void Scope::DeclareVariable(std::string_view name)
{
    if (IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Variable was '{}' declared before.", name));
    }

    variables_.insert(std::string{name});
}

void Scope::DeclareVariable(std::string_view name, std::shared_ptr<Type> type)
{
    DeclareVariable(name);
    SetVariableType(name, type);
}

bool Scope::IsVariableDeclared(std::string_view name) const
{
    return variables_.contains(std::string{name});
}

void Scope::SetVariableType(std::string_view name, std::shared_ptr<Type> type)
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot set type of undeclared variable '{}'.", name));
    }

    if (IsVariableTypeSet(name))
    {
        throw ScopeError(std::format("Type of variable '{}' was set before.", name));
    }

    variable_types_.insert(std::make_pair(name, type));
}

bool Scope::IsVariableTypeSet(std::string_view name) const
{
    return variable_types_.contains(std::string{name});
}

std::shared_ptr<Type> Scope::GetVariableType(std::string_view name) const
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot get type of undeclared variable '{}'.", name));
    }

    if (!IsVariableTypeSet(name))
    {
        throw ScopeError(std::format("Type of variable '{}' is undefined.", name));
    }

    return variable_types_.at(std::string{name});
}

void Scope::SetLLVMValue(std::string_view name, llvm::Value* llvm_value)
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot set LLVM Value of undeclared variable '{}'.", name));
    }

    if (llvm_values_.contains(std::string{name}))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' was set before.", name));
    }

    llvm_values_.insert(std::make_pair(name, llvm_value));
}

llvm::Value* Scope::GetLLVMValue(std::string_view name)
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot get LLVM Value of undeclared variable '{}'.", name));
    }

    if (!llvm_values_.contains(std::string{name}))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' is undefined.", name));
    }

    return llvm_values_.at(std::string{name});
}

void Scope::DeclareType(std::string_view name)
{
    if (IsTypeDeclared(std::string{name}))
    {
        throw ScopeError(std::format("Type '{}' was declared before.", name));
    }

    types_.insert(std::string{name});
}

bool Scope::IsTypeDeclared(std::string_view name) const
{
    return types_.contains(std::string{name});
}

void Scope::DefineType(std::string_view name, std::shared_ptr<Type> type)
{
    if (!IsTypeDeclared(name))
    {
        throw ScopeError(std::format("Type '{}' is undefined.", name));
    }

    if (IsTypeDefined(name))
    {
        throw ScopeError(std::format("Type '{}' was defined before.", name));
    }

    type_definitions_.insert({std::string{name}, type});
}

bool Scope::IsTypeDefined(std::string_view name) const
{
    return type_definitions_.contains(std::string{name});
}

std::shared_ptr<Type> Scope::GetTypeDefinition(std::string_view name) const
{
    if (!IsTypeDefined(name))
    {
        throw ScopeError(std::format("Type '{}' is undefined.", name));
    }

    return type_definitions_.at(std::string{name});
}

void Scope::Clear()
{
    llvm_values_.clear();
    types_.clear();
    variables_.clear();
}

const std::unordered_set<std::string>& Scope::GetVariables() const
{
    return variables_;
}

const std::unordered_set<std::string>& Scope::GetTypes() const
{
    return types_;
}

void Scope::UpdateTypes(const Scope& other)
{
    for (const auto& [name, definition] : other.type_definitions_)
    {
        DeclareType(name);
        DefineType(name, definition);
    }
}

void Scope::UpdateVariables(const Scope& other)
{
    for (const auto& [name, type] : other.variable_types_)
    {
        DeclareVariable(name, type);
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
