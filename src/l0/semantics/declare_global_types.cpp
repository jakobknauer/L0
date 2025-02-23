#include "l0/semantics/declare_global_types.h"

#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

void DeclareGlobalTypes(Module& module)
{
    for (const auto& type_declaration : module.global_type_declarations)
    {
        DeclareGlobaType(module, *type_declaration);
    }
}

void DeclareGlobaType(Module& module, const TypeDeclaration& type_declaration)
{
    module.globals->DeclareType(type_declaration.identifier);

    if (dynamic_pointer_cast<StructExpression>(type_declaration.definition))
    {
        auto type = std::make_shared<StructType>(
            type_declaration.identifier, std::make_shared<StructMemberList>(), TypeQualifier::Constant
        );
        type_declaration.type = type;
        module.globals->DefineType(type_declaration.identifier, type);
    }
    else if (dynamic_pointer_cast<EnumExpression>(type_declaration.definition))
    {
        auto type = std::make_shared<EnumType>(
            type_declaration.identifier, std::make_shared<EnumMemberList>(), TypeQualifier::Constant
        );
        type_declaration.type = type;
        module.globals->DefineType(type_declaration.identifier, type);
    }
    else
    {
        throw SemanticError("Encountered an unsupported type declaration.");
    }
}

}  // namespace l0::detail
