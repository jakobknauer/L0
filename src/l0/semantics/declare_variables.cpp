#include "l0/semantics/declare_variables.h"

#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

void DeclareGlobalVariables(Module& module)
{
    TypeResolver type_resolver{module};
    for (auto declaration : module.global_declarations)
    {
        DeclareGlobalVariable(module, *declaration, type_resolver);
    }
}

void DeclareGlobalVariable(Module& module, const Declaration& declaration, TypeResolver& type_resolver)
{
    if (module.globals->IsVariableDeclared(declaration.identifier))
    {
        throw SemanticError(
            std::format("Duplicate declaration of global variable '{}'.", declaration.identifier.ToString())
        );
    }

    auto function = dynamic_pointer_cast<Function>(declaration.initializer);
    if (!function)
    {
        throw SemanticError(
            std::format("Initializer of global variable must be a function.", declaration.identifier.ToString())
        );
    }

    if (!declaration.annotation)
    {
        throw SemanticError("Types of globals cannot be inferred.");
    }

    if (declaration.annotation->mutability == TypeAnnotationQualifier::Mutable)
    {
        throw SemanticError("Globals may not be declared mutable.");
    }

    std::shared_ptr<Type> type = type_resolver.Convert(*declaration.annotation, declaration.identifier.GetPrefix());

    module.globals->DeclareVariable(declaration.identifier);
    module.globals->SetVariableType(declaration.identifier, type);

    declaration.scope = module.globals;

    function->global_name =
        declaration.identifier == "main" ? "main" : std::format("__fn__{}", declaration.identifier.ToString());
    module.callables.push_back(function);
}

}  // namespace l0::detail
