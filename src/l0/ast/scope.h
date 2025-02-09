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
    void DeclareVariable(Identifier identifier);
    void DeclareVariable(Identifier identifier, std::shared_ptr<Type> type);
    bool IsVariableDeclared(Identifier identifier) const;

    void SetVariableType(Identifier identifier, std::shared_ptr<Type> type);
    bool IsVariableTypeSet(Identifier identifier) const;
    std::shared_ptr<Type> GetVariableType(Identifier identifier) const;

    void SetLLVMValue(Identifier identifier, llvm::Value* llvm_value);
    llvm::Value* GetLLVMValue(Identifier identifier) const;

    void DeclareType(Identifier identifier);
    bool IsTypeDeclared(Identifier identifier) const;
    void DefineType(Identifier identifier, std::shared_ptr<Type> type);
    bool IsTypeDefined(Identifier identifier) const;
    std::shared_ptr<Type> GetTypeDefinition(Identifier identifier) const;

    void Clear();
    const std::unordered_set<Identifier>& GetVariables() const;
    const std::unordered_set<Identifier>& GetTypes() const;

    void UpdateTypes(const Scope& other);
    void UpdateVariables(const Scope& other);

   private:
    std::unordered_set<Identifier> variables_;
    std::unordered_map<Identifier, std::shared_ptr<Type>> variable_types_;
    std::unordered_map<Identifier, llvm::Value*> llvm_values_;

    std::unordered_set<Identifier> types_;
    std::unordered_map<Identifier, std::shared_ptr<Type>> type_definitions_;
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
