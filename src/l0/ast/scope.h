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
    void Declare(const std::string& name);
    void Declare(const std::string& name, std::shared_ptr<Type> type);
    bool IsDeclared(const std::string& name) const;

    void SetType(const std::string& name, std::shared_ptr<Type> type);
    bool IsTypeSet(const std::string& name) const;
    std::shared_ptr<Type> GetType(const std::string& name);

    void SetLLVMValue(const std::string& name, llvm::Value* llvm_value);
    llvm::Value* GetLLVMValue(const std::string& name);

    void Clear();
    const std::unordered_set<std::string>& GetVariables() const;

   private:
    std::unordered_set<std::string> variables_;
    std::unordered_map<std::string, std::shared_ptr<Type>> types_;
    std::unordered_map<std::string, llvm::Value*> llvm_values_;
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
