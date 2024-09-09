#include "l0/semantics/global_scope_builder.h"

#include "l0/ast/statement.h"
#include "l0/ast/type_expression.h"
#include "l0/semantics/semantic_error.h"
#include "l0/types/types.h"

namespace l0
{

GlobalScopeBuilder::GlobalScopeBuilder(Module& module) : module_{module}, type_resolver_{module}
{
    // TODO declare basic types as external types - requires 'type resolution' in type_annotation_converter
    module_.globals->DeclareType("()");
    module_.globals->DeclareType("Boolean");
    module_.globals->DeclareType("Integer");
    module_.globals->DeclareType("String");

    module_.globals->DefineType("()", std::make_shared<UnitType>(TypeQualifier::Constant));
    module_.globals->DefineType("Boolean", std::make_shared<BooleanType>(TypeQualifier::Constant));
    module_.globals->DefineType("Integer", std::make_shared<IntegerType>(TypeQualifier::Constant));
    module_.globals->DefineType("String", std::make_shared<StringType>(TypeQualifier::Constant));
}

void GlobalScopeBuilder::Run()
{
    std::vector<std::shared_ptr<TypeDeclaration>> type_declarations{};
    std::vector<std::shared_ptr<Declaration>> declarations{};

    for (auto& statement : *module_.statements)
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
        member->type = type_resolver_.Convert(*member_declaration->annotation);
        member->default_initializer = member_declaration->initializer;
        struct_type->members->push_back(member);
    }
}

void GlobalScopeBuilder::DeclareVariable(std::shared_ptr<Declaration> declaration)
{
    if (module_.globals->IsVariableDeclared(declaration->variable))
    {
        throw SemanticError(std::format("Duplicate declaration of global variable '{}'.", declaration->variable));
    }
    module_.globals->DeclareVariable(declaration->variable);

    auto initializer = declaration->initializer;
    bool initializer_is_literal = dynamic_pointer_cast<IntegerLiteral>(initializer) ||
                                  dynamic_pointer_cast<StringLiteral>(initializer) ||
                                  dynamic_pointer_cast<Function>(initializer);
    if (!initializer_is_literal)
    {
        throw SemanticError(
            std::format("Initializer of global variable must be a literal or function.", declaration->variable)
        );
    }

    std::shared_ptr<Type> type;
    if (declaration->annotation)
    {
        if (declaration->annotation->mutability == TypeAnnotationQualifier::Mutable)
        {
            throw SemanticError(std::format("Globals may not be declared mutable."));
        }
        type = type_resolver_.Convert(*declaration->annotation);
    }
    else
    {
        if (dynamic_pointer_cast<IntegerLiteral>(declaration->initializer))
        {
            type = module_.globals->GetTypeDefinition("Integer");
        }
        else if (dynamic_pointer_cast<StringLiteral>(declaration->initializer))
        {
            type = module_.globals->GetTypeDefinition("String");
        }
        else if (auto function = dynamic_pointer_cast<Function>(declaration->initializer))
        {
            type = type_resolver_.Convert(*function);
        }
    }

    module_.globals->SetVariableType(declaration->variable, type);
    declaration->scope = module_.globals;
};

}  // namespace l0
