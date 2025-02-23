#ifndef L0_SEMANTICS_SEMANTICS_H
#define L0_SEMANTICS_SEMANTICS_H

#include "l0/ast/module.h"

namespace l0
{

void DeclareGlobalTypes(Module& module);
void FillGlobalTypes(Module& module);
void DeclareGlobalVariables(Module& module);
void BuildAndResolveLocalScopes(Module& module);
void CheckTypes(Module& module);
void CheckReturnStatements(Module& module);
void CheckReferences(Module& module);

}  // namespace l0

#endif
