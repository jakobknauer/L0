#ifndef L0_AST_SCOPE_H
#define L0_AST_SCOPE_H

#include <llvm/IR/Value.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "l0/types/types.h"

namespace l0
{

class Scope
{
   public:
    void DeclareVariable(const std::string& name);
    void DeclareVariable(const std::string& name, std::shared_ptr<Type> type);
    bool IsVariableDeclared(const std::string& name) const;

    void SetVariableType(const std::string& name, std::shared_ptr<Type> type);
    bool IsVariableTypeSet(const std::string& name) const;
    std::shared_ptr<Type> GetVariableType(const std::string& name) const;

    void SetLLVMValue(const std::string& name, llvm::Value* llvm_value);
    llvm::Value* GetLLVMValue(const std::string& name);

    void DeclareType(const std::string& name);
    bool IsTypeDeclared(const std::string& name) const;
    void DefineType(const std::string& name, std::shared_ptr<Type> type);
    bool IsTypeDefined(const std::string& name) const;
    std::shared_ptr<Type> GetTypeDefinition(const std::string& name) const;

    void Clear();
    const std::unordered_set<std::string>& GetVariables() const;
    const std::unordered_set<std::string>& GetTypes() const;

   private:
    std::unordered_set<std::string> variables_;
    std::unordered_map<std::string, std::shared_ptr<Type>> variable_types_;
    std::unordered_map<std::string, llvm::Value*> llvm_values_;

    std::unordered_set<std::string> types_;
    std::unordered_map<std::string, std::shared_ptr<Type>> type_definitions_;
};

class ScopeError
{
   public:
    ScopeError(const std::string& message);

    std::string GetMessage() const;

   private:
    const std::string message_;
};

}  // namespace l0

#endif
