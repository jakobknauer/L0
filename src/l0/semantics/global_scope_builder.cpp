#include "l0/semantics/global_scope_builder.h"

#include <format>

#include "l0/ast/statement.h"
#include "l0/ast/type_expression.h"
#include "l0/common/constants.h"
#include "l0/semantics/semantic_error.h"
#include "l0/types/types.h"

namespace l0
{

GlobalScopeBuilder::GlobalScopeBuilder(Module& module)
    : module_{module}
{
    // TODO declare basic types as external types - requires 'type resolution' in type_annotation_converter
    module_.globals->DeclareType(Typename::Unit);
    module_.globals->DeclareType(Typename::Boolean);
    module_.globals->DeclareType(Typename::Integer);
    module_.globals->DeclareType(Typename::Character);
    module_.globals->DeclareType(Typename::CString);

    module_.globals->DefineType(Typename::Unit, std::make_shared<UnitType>(TypeQualifier::Constant));
    module_.globals->DefineType(Typename::Boolean, std::make_shared<BooleanType>(TypeQualifier::Constant));
    module_.globals->DefineType(Typename::Integer, std::make_shared<IntegerType>(TypeQualifier::Constant));
    auto C8 = std::make_shared<CharacterType>(TypeQualifier::Constant);
    module_.globals->DefineType(Typename::Character, C8);
    module_.globals->DefineType(Typename::CString, std::make_shared<ReferenceType>(C8, TypeQualifier::Constant));
}

void GlobalScopeBuilder::Run()
{
    std::vector<std::shared_ptr<TypeDeclaration>> type_declarations{};
    std::vector<std::shared_ptr<Declaration>> declarations{};

    for (auto& statement : module_.statements->statements)
    {
        if (auto declaration = dynamic_pointer_cast<Declaration>(statement))
        {
            declarations.push_back(declaration);
        }
        else if (auto type_declaration = dynamic_pointer_cast<TypeDeclaration>(statement))
        {
            type_declarations.push_back(type_declaration);
        }
        else
        {
            throw SemanticError("Only variable and type declarations are allowed as global statements.");
        }
    }

    for (auto type_declaration : type_declarations)
    {
        DeclareType(type_declaration);
    }
    for (auto type_declaration : type_declarations)
    {
        FillTypeDetails(type_declaration);
    }
    for (auto declaration : declarations)
    {
        DeclareVariable(declaration);
    }
}

void GlobalScopeBuilder::DeclareType(std::shared_ptr<TypeDeclaration> type_declaration)
{
    module_.globals->DeclareType(type_declaration->name);

    auto type = std::make_shared<StructType>(
        type_declaration->name, std::make_shared<StructMemberList>(), TypeQualifier::Constant
    );
    module_.globals->DefineType(type_declaration->name, type);
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
        }
        else
        {
            member->type = type_resolver_.Convert(*member_declaration->annotation);
        }

        if (auto function = std::dynamic_pointer_cast<Function>(member->default_initializer))
        {
            function->global_name = std::format("__memberfct_{}::{}", struct_type->name, member->name);
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

    module_.global_declarations.push_back(declaration);
    declaration->scope = module_.globals;

    function->global_name = declaration->variable;
    module_.callables.push_back(function);
}

}  // namespace l0
