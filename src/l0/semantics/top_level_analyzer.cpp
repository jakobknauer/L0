#include "l0/semantics/top_level_analyzer.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

TopLevelAnalyzer::TopLevelAnalyzer(Module& module)
    : module_{module}
{
}

void TopLevelAnalyzer::Run()
{
    for (auto& statement : module_.statements->statements)
    {
        if (auto declaration = dynamic_pointer_cast<Declaration>(statement))
        {
            module_.global_declarations.push_back(declaration);
        }
        else if (auto type_declaration = dynamic_pointer_cast<TypeDeclaration>(statement))
        {
            module_.global_type_declarations.push_back(type_declaration);
        }
        else
        {
            throw SemanticError("Only variable and type declarations are allowed as global statements.");
        }
    }

    for (auto type_declaration : module_.global_type_declarations)
    {
        DeclareType(type_declaration);
    }
}

void TopLevelAnalyzer::DeclareType(std::shared_ptr<TypeDeclaration> type_declaration)
{
    module_.globals->DeclareType(type_declaration->name);

    auto type = std::make_shared<StructType>(
        type_declaration->name, std::make_shared<StructMemberList>(), TypeQualifier::Constant
    );
    module_.globals->DefineType(type_declaration->name, type);
}

}  // namespace l0
