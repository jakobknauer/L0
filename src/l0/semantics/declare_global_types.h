#ifndef L0_SEMANTICS_DECLARE_GLOBAL_TYPES_H
#define L0_SEMANTICS_DECLARE_GLOBAL_TYPES_H

#include "l0/ast/module.h"
#include "l0/ast/statement.h"

namespace l0::detail
{

void DeclareGlobalTypes(Module& module);
void DeclareGlobaType(Module& module, const TypeDeclaration& type_declaration);

}  // namespace l0::detail

#endif
