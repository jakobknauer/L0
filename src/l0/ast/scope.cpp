#include "l0/ast/scope.h"

namespace l0
{

void Scope::DeclareVariable(const std::string& name)
{
    if (IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Variable was '{}' declared before.", name));
    }

    variables_.insert(name);
}

void Scope::DeclareVariable(const std::string& name, std::shared_ptr<Type> type)
{
    DeclareVariable(name);
    SetVariableType(name, type);
}

bool Scope::IsVariableDeclared(const std::string& name) const
{
    return variables_.contains(name);
}

void Scope::SetVariableType(const std::string& name, std::shared_ptr<Type> type)
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

bool Scope::IsVariableTypeSet(const std::string& name) const
{
    return variable_types_.contains(name);
}

std::shared_ptr<Type> Scope::GetVariableType(const std::string& name) const
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot get type of undeclared variable '{}'.", name));
    }

    if (!IsVariableTypeSet(name))
    {
        throw ScopeError(std::format("Type of variable '{}' is undefined.", name));
    }

    return variable_types_.at(name);
}

void Scope::SetLLVMValue(const std::string& name, llvm::Value* llvm_value)
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot set LLVM Value of undeclared variable '{}'.", name));
    }

    if (llvm_values_.contains(name))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' was set before.", name));
    }

    llvm_values_.insert(std::make_pair(name, llvm_value));
}

llvm::Value* Scope::GetLLVMValue(const std::string& name)
{
    if (!IsVariableDeclared(name))
    {
        throw ScopeError(std::format("Cannot get LLVM Value of undeclared variable '{}'.", name));
    }

    if (!llvm_values_.contains(name))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' is undefined.", name));
    }

    return llvm_values_.at(name);
}

void Scope::DeclareType(const std::string& name)
{
    if (IsTypeDeclared(name))
    {
        throw ScopeError(std::format("Type '{}' was declared before.", name));
    }

    types_.insert(name);
}

bool Scope::IsTypeDeclared(const std::string& name) const
{
    return types_.contains(name);
}

void Scope::DefineType(const std::string& name, std::shared_ptr<Type> type)
{
    if (!IsTypeDeclared(name))
    {
        throw ScopeError(std::format("Type '{}' is undefined.", name));
    }

    if (IsTypeDefined(name))
    {
        throw ScopeError(std::format("Type '{}' was defined before.", name));
    }

    type_definitions_.insert({name, type});
}

bool Scope::IsTypeDefined(const std::string& name) const
{
    return type_definitions_.contains(name);
}

std::shared_ptr<Type> Scope::GetTypeDefinition(const std::string& name) const
{
    if (!IsTypeDefined(name))
    {
        throw ScopeError(std::format("Type '{}' is undefined.", name));
    }

    return type_definitions_.at(name);
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

ScopeError::ScopeError(const std::string& message)
    : message_{message}
{
}

std::string ScopeError::GetMessage() const
{
    return message_;
}

}  // namespace l0
