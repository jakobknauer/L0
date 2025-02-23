#include "l0/semantics/top_level_analyzer.h"

#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

TopLevelAnalyzer::TopLevelAnalyzer(Module& module)
    : module_{module}
{
}

void TopLevelAnalyzer::Run()
{
    for (auto type_declaration : module_.global_type_declarations)
    {
        DeclareType(type_declaration);
    }
}

void TopLevelAnalyzer::DeclareType(std::shared_ptr<TypeDeclaration> type_declaration)
{
    module_.globals->DeclareType(type_declaration->identifier);

    if (dynamic_pointer_cast<StructExpression>(type_declaration->definition))
    {
        auto type = std::make_shared<StructType>(
            type_declaration->identifier, std::make_shared<StructMemberList>(), TypeQualifier::Constant
        );
        type_declaration->type = type;
        module_.globals->DefineType(type_declaration->identifier, type);
    }
    else if (dynamic_pointer_cast<EnumExpression>(type_declaration->definition))
    {
        auto type = std::make_shared<EnumType>(
            type_declaration->identifier, std::make_shared<EnumMemberList>(), TypeQualifier::Constant
        );
        type_declaration->type = type;
        module_.globals->DefineType(type_declaration->identifier, type);
    }
    else
    {
        throw SemanticError("Encountered an unsupported type declaration.");
    }
}

}  // namespace l0::detail
