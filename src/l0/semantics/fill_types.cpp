#include "l0/semantics/fill_types.h"

#include <ranges>

#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

void FillTypeDetails(Module& module)
{
    TypeResolver type_resolver{module};

    for (auto type_declaration : module.global_type_declarations)
    {
        if (auto struct_expression = dynamic_pointer_cast<StructExpression>(type_declaration->definition))
        {
            auto struct_type = dynamic_pointer_cast<StructType>(type_declaration->type);
            if (!struct_type)
            {
                throw SemanticError("Expected type of type declaration to be of struct type.");
            }
            FillStructDetails(module, struct_type, *struct_expression, type_resolver);
        }
        else if (auto enum_expression = dynamic_pointer_cast<EnumExpression>(type_declaration->definition))
        {
            auto enum_type = dynamic_pointer_cast<EnumType>(type_declaration->type);
            if (!enum_type)
            {
                throw SemanticError("Expected type of type declaration to be enum type.");
            }
            FillEnumDetails(module, enum_type, *enum_expression);
        }
        else
        {
            throw SemanticError("Only struct or enum expressions are allowed as type definitions.");
        }
    }
}

void FillStructDetails(
    Module& module, std::shared_ptr<StructType> type, const StructExpression& definition, TypeResolver& type_resolver
)
{
    for (auto& member_declaration : *definition.members)
    {
        auto member = std::make_shared<StructMember>();
        member->name = member_declaration->identifier.ToString();
        member->default_initializer = member_declaration->initializer;

        if (auto method_annotation = dynamic_pointer_cast<MethodTypeAnnotation>(member_declaration->annotation))
        {
            member->type = type_resolver.Convert(*method_annotation->function_type, type->identifier.GetPrefix());
            member->is_method = true;
            member->is_static = true;
        }
        else
        {
            member->type = type_resolver.Convert(*member_declaration->annotation, type->identifier.GetPrefix());
        }

        if (member->default_initializer)
        {
            member->default_initializer_global_name = std::format("{}::{}", type->identifier.ToString(), member->name);
            module.globals->DeclareVariable(*member->default_initializer_global_name);
            module.globals->SetVariableType(*member->default_initializer_global_name, member->type);
        }
        if (auto function = std::dynamic_pointer_cast<Function>(member->default_initializer))
        {
            function->global_name = std::format("__fn__{}::{}", type->identifier.ToString(), member->name);
            module.callables.push_back(function);
        }

        type->members->push_back(member);
    }
}

void FillEnumDetails(Module& module, std::shared_ptr<EnumType> type, const EnumExpression& definition)
{
    for (const auto& [index, member] : *definition.members | std::views::enumerate)
    {
        type->members->push_back(std::make_shared<EnumMember>(member->name));

        Identifier full_member_name = type->identifier + member->name;

        module.globals->DeclareVariable(full_member_name);
        module.globals->SetVariableType(full_member_name, type);
    }
}

}  // namespace l0::detail
