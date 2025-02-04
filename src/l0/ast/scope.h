#ifndef L0_AST_SCOPE_H
#define L0_AST_SCOPE_H

#include <llvm/IR/Value.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "l0/ast/identifier.h"
#include "l0/types/types.h"

namespace l0
{

class Scope
{
   public:
    void DeclareVariable(Identifier name);
    void DeclareVariable(Identifier name, std::shared_ptr<Type> type);
    bool IsVariableDeclared(Identifier name) const;

    void SetVariableType(Identifier name, std::shared_ptr<Type> type);
    bool IsVariableTypeSet(Identifier name) const;
    std::shared_ptr<Type> GetVariableType(Identifier name) const;

    void SetLLVMValue(Identifier name, llvm::Value* llvm_value);
    llvm::Value* GetLLVMValue(Identifier name) const;

    void DeclareType(std::string_view name);
    bool IsTypeDeclared(std::string_view name) const;
    void DefineType(std::string_view name, std::shared_ptr<Type> type);
    bool IsTypeDefined(std::string_view name) const;
    std::shared_ptr<Type> GetTypeDefinition(std::string_view name) const;

    void Clear();
    const std::unordered_set<Identifier>& GetVariables() const;
    const std::unordered_set<std::string>& GetTypes() const;

    void UpdateTypes(const Scope& other);
    void UpdateVariables(const Scope& other);

   private:
    std::unordered_set<Identifier> variables_;
    std::unordered_map<Identifier, std::shared_ptr<Type>> variable_types_;
    std::unordered_map<Identifier, llvm::Value*> llvm_values_;

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
