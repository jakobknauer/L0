#include "l0/semantics/global_scope_builder.h"

#include <format>

#include "l0/ast/statement.h"
#include "l0/ast/type_expression.h"
#include "l0/semantics/semantic_error.h"
#include "l0/types/types.h"

namespace l0
{

GlobalScopeBuilder::GlobalScopeBuilder(Module& module)
    : module_{module}
{
}

void GlobalScopeBuilder::Run()
{
    for (auto type_declaration : module_.global_type_declarations)
    {
        FillTypeDetails(type_declaration);
    }
    for (auto declaration : module_.global_declarations)
    {
        DeclareVariable(declaration);
    }
}

void GlobalScopeBuilder::FillTypeDetails(std::shared_ptr<TypeDeclaration> type_declaration)
{
    auto struct_expression = dynamic_pointer_cast<StructExpression>(type_declaration->definition);
    if (!struct_expression)
    {
        throw SemanticError("Only structs expressions are allowed as type definitions.");
    }

    auto struct_type = dynamic_pointer_cast<StructType>(module_.globals->GetTypeDefinition(type_declaration->name));
    for (auto& member_declaration : *struct_expression->members)
    {
        auto member = std::make_shared<StructMember>();
        member->name = member_declaration->variable;
        member->default_initializer = member_declaration->initializer;

        if (auto method_annotation = dynamic_pointer_cast<MethodTypeAnnotation>(member_declaration->annotation))
        {
            member->type = type_resolver_.Convert(*method_annotation->function_type);
            member->is_method = true;
            member->is_static = true;
        }
        else
        {
            member->type = type_resolver_.Convert(*member_declaration->annotation);
        }

        if (auto function = std::dynamic_pointer_cast<Function>(member->default_initializer))
        {
            function->global_name = std::format("{}::{}", struct_type->name, member->name);
            module_.callables.push_back(function);

            module_.globals->DeclareVariable(function->global_name.value());
            module_.globals->SetVariableType(function->global_name.value(), type_resolver_.Convert(*function));
        }

        struct_type->members->push_back(member);
    }
}

void GlobalScopeBuilder::DeclareVariable(std::shared_ptr<Declaration> declaration)
{
    if (module_.globals->IsVariableDeclared(declaration->variable))
    {
        throw SemanticError(std::format("Duplicate declaration of global variable '{}'.", declaration->variable));
    }

    auto function = dynamic_pointer_cast<Function>(declaration->initializer);
    if (!function)
    {
        throw SemanticError(std::format("Initializer of global variable must be a function.", declaration->variable));
    }

    if (!declaration->annotation)
    {
        throw SemanticError(std::format("Types of globals cannot be inferred."));
    }

    if (declaration->annotation->mutability == TypeAnnotationQualifier::Mutable)
    {
        throw SemanticError(std::format("Globals may not be declared mutable."));
    }

    std::shared_ptr<Type> type = type_resolver_.Convert(*declaration->annotation);

    module_.globals->DeclareVariable(declaration->variable);
    module_.globals->SetVariableType(declaration->variable, type);

    declaration->scope = module_.globals;

    function->global_name = declaration->variable;
    module_.callables.push_back(function);
}

}  // namespace l0
