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
    if (auto struct_expression = dynamic_pointer_cast<StructExpression>(type_declaration->definition))
    {
        auto struct_type = dynamic_pointer_cast<StructType>(type_declaration->type);
        if (!struct_type)
        {
            throw SemanticError("Expected type of type declaration to be of struct type.");
        }
        FillStructDetails(struct_type, struct_expression);
    }
    else if (auto enum_expression = dynamic_pointer_cast<EnumExpression>(type_declaration->definition))
    {
        auto enum_type = dynamic_pointer_cast<EnumType>(type_declaration->type);
        if (!enum_type)
        {
            throw SemanticError("Expected type of type declaration to be of enum type.");
        }
        FillEnumDetails(enum_type, enum_expression);
    }
    else
    {
        throw SemanticError("Only struct or enum expressions are allowed as type definitions.");
    }
}

void GlobalScopeBuilder::FillStructDetails(
    std::shared_ptr<StructType> type, std::shared_ptr<StructExpression> definition
)
{
    for (auto& member_declaration : *definition->members)
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

        if (member->default_initializer)
        {
            member->default_initializer_global_name = std::format("{}::{}", type->name, member->name);
            module_.globals->DeclareVariable(*member->default_initializer_global_name);
            module_.globals->SetVariableType(*member->default_initializer_global_name, member->type);
        }
        if (auto function = std::dynamic_pointer_cast<Function>(member->default_initializer))
        {
            function->global_name = std::format("__fn__{}::{}", type->name, member->name);
            module_.callables.push_back(function);
        }

        type->members->push_back(member);
    }
}

void GlobalScopeBuilder::FillEnumDetails(std::shared_ptr<EnumType> type, std::shared_ptr<EnumExpression> definition)
{
    for (auto& member : *definition->members)
    {
        type->members->push_back(std::make_shared<EnumMember>(member->name));
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

    function->global_name = declaration->variable == "main" ? "main" : std::format("__fn__{}", declaration->variable);
    module_.callables.push_back(function);
}

}  // namespace l0
