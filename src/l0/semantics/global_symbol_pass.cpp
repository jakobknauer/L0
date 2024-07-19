#include "l0/semantics/global_symbol_pass.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

GlobalSymbolPass::GlobalSymbolPass(Module& module) : module_{module} {}

void GlobalSymbolPass::Run()
{
    module_.globals->Clear();

    for (auto& statement : *module_.statements)
    {
        auto declaration = dynamic_pointer_cast<Declaration>(statement);

        if (module_.globals->IsDeclared(declaration->variable))
        {
            throw SemanticError(std::format("Duplicate declaration of global variable '{}'.", declaration->variable));
        }

        std::shared_ptr<Type> type;
        if (declaration->annotation)
        {
            type = converter_.Convert(*declaration->annotation);
        }
        else
        {
            if (dynamic_pointer_cast<IntegerLiteral>(declaration->initializer))
            {
                type = std::make_shared<IntegerType>();
            }
            else if (dynamic_pointer_cast<StringLiteral>(declaration->initializer))
            {
                type = std::make_shared<StringType>();
            }
            else if (auto function = dynamic_pointer_cast<Function>(declaration->initializer))
            {
                auto function_type = std::make_shared<FunctionType>();
                function_type->return_type = converter_.Convert(*function->return_type_annotation);
                for (auto& parameter : *function->parameters)
                {
                    function_type->parameters->push_back(converter_.Convert(*parameter->annotation));
                }
                type = function_type;
            }
        }

        module_.globals->Declare(declaration->variable, type);
        declaration->scope = module_.globals;
    }
}

}  // namespace l0
