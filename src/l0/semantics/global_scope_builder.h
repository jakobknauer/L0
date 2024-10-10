#ifndef L0_SEMANTICS_GLOBAL_SCOPE_BUILDER_H
#define L0_SEMANTICS_GLOBAL_SCOPE_BUILDER_H

#include <memory>

#include "l0/ast/module.h"
#include "l0/semantics/type_resolver.h"

namespace l0
{

class GlobalScopeBuilder
{
   public:
    GlobalScopeBuilder(Module& module);

    void Run();

   private:
    void FillTypeDetails(std::shared_ptr<TypeDeclaration> type_declaration);
    void DeclareVariable(std::shared_ptr<Declaration> declaration);

    Module& module_;
    detail::TypeResolver type_resolver_{module_};
};

}  // namespace l0

#endif
