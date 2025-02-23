#ifndef L0_SEMANTICS_DECLARE_VARIABLES_H
#define L0_SEMANTICS_DECLARE_VARIABLES_H

#include "l0/ast/module.h"
#include "l0/semantics/type_resolver.h"

namespace l0::detail
{

void DeclareGlobalVariables(Module& module);
void DeclareGlobalVariable(Module& module, const Declaration& declaration, TypeResolver& type_resolver);

}  // namespace l0::detail

#endif
