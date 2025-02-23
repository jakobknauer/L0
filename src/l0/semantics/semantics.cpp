#include "l0/semantics/semantics.h"

#include "l0/semantics/declare_variables.h"
#include "l0/semantics/fill_types.h"
#include "l0/semantics/reference_pass.h"
#include "l0/semantics/resolver.h"
#include "l0/semantics/return_statement_pass.h"
#include "l0/semantics/top_level_analyzer.h"
#include "l0/semantics/typechecker.h"

namespace l0
{

void DeclareGlobalTypes(Module& module)
{
    detail::TopLevelAnalyzer{module}.Run();
}

void FillGlobalTypes(Module& module)
{
    detail::FillTypeDetails(module);
}

void DeclareGlobalVariables(Module& module)
{
    detail::DeclareGlobalVariables(module);
}

void BuildAndResolveLocalScopes(Module& module)
{
    detail::Resolver{module}.Run();
}

void CheckTypes(Module& module)
{
    detail::Typechecker{module}.Run();
}

void CheckReturnStatements(Module& module)
{
    detail::ReturnStatementPass{module}.Run();
}

void CheckReferences(Module& module)
{
    detail::ReferencePass{module}.Run();
}

}  // namespace l0
