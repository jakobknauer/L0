#include "l0/ast/scope.h"

namespace l0
{

void Scope::Declare(const std::string& name)
{
    if (IsDeclared(name))
    {
        throw ScopeError(std::format("Variable '{}' declared before.", name));
    }

    variables_.insert(name);
}

void Scope::Declare(const std::string& name, std::shared_ptr<Type> type)
{
    Declare(name);
    SetType(name, type);
}

bool Scope::IsDeclared(const std::string& name) const { return variables_.contains(name); }

void Scope::SetType(const std::string& name, std::shared_ptr<Type> type)
{
    if (!IsDeclared(name))
    {
        throw ScopeError(std::format("Cannot set type of undeclared variable '{}'.", name));
    }

    if (types_.contains(name))
    {
        throw ScopeError(std::format("Type of variable '{}' was set before.", name));
    }

    types_.insert(std::make_pair(name, type));
}

std::shared_ptr<Type> Scope::GetType(const std::string& name)
{
    if (!IsDeclared(name))
    {
        throw ScopeError(std::format("Cannot get type of undeclared variable '{}'.", name));
    }

    if (!types_.contains(name))
    {
        throw ScopeError(std::format("Type of variable '{}' is undefined.", name));
    }

    return types_.at(name);
}

void Scope::SetLLVMValue(const std::string& name, llvm::Value* llvm_value)
{
    if (!IsDeclared(name))
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
    if (!IsDeclared(name))
    {
        throw ScopeError(std::format("Cannot get LLVM Value of undeclared variable '{}'.", name));
    }

    if (!llvm_values_.contains(name))
    {
        throw ScopeError(std::format("LLVM Value of variable '{}' is undefined.", name));
    }

    return llvm_values_.at(name);
}

void Scope::Clear()
{
    llvm_values_.clear();
    types_.clear();
    variables_.clear();
}

const std::unordered_set<std::string>& Scope::GetVariables() const { return variables_; }

ScopeError::ScopeError(const std::string& message) : message_{message} {}

std::string ScopeError::GetMessage() const { return message_; }

}  // namespace l0
