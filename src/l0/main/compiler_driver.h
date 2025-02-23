#ifndef L0_MAIN_COMPILER_DRIVER_H
#define L0_MAIN_COMPILER_DRIVER_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <filesystem>
#include <memory>
#include <vector>

#include "l0/ast/module.h"

namespace l0
{

class CompilerDriver
{
   public:
    void LoadModules(const std::vector<std::filesystem::path>& paths);
    void DeclareEnvironmentSymbols();
    void DeclareGlobalTypes();
    void DeclareExternalTypes();
    void DefineGlobalSymbols();
    void DeclareExternalVariables();
    void RunSemanticAnalysis();
    void GenerateIR();
    void StoreIR();

   private:
    void LoadModule(const std::filesystem::path& input_path);
    void FillEnvironmentScope(Module& module);
    void SemanticCheckModule(Module& module);
    void GenerateIRForModule(Module& module);
    void StoreModuleIR(Module& module);

    std::vector<std::shared_ptr<Module>> modules_{};
    llvm::LLVMContext context_{};
};

}  // namespace l0

#endif
