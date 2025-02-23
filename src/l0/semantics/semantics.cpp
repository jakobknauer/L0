#include "l0/semantics/semantics.h"

#include "l0/semantics/global_scope_builder.h"
#include "l0/semantics/reference_pass.h"
#include "l0/semantics/resolver.h"
#include "l0/semantics/return_statement_pass.h"
#include "l0/semantics/top_level_analyzer.h"
#include "l0/semantics/typechecker.h"

namespace l0
{

void RunTopLevelAnalysis(Module& module)
{
    detail::TopLevelAnalyzer{module}.Run();
}

void BuildGlobalScope(Module& module)
{
    detail::GlobalScopeBuilder{module}.Run();
}

void BuildAndResolveLocalScopes(Module& module)
{
    detail::Resolver{module}.Run();
}

void RunTypecheck(Module& module)
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
