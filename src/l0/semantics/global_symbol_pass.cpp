#include "l0/semantics/global_symbol_pass.h"

#include <memory>
#include <vector>

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
            if (declaration->annotation->mutability == TypeAnnotationQualifier::Mutable)
            {
                throw SemanticError(std::format("Globals may not be declared mutable."));
            }
            type = converter_.Convert(*declaration->annotation);
        }
        else
        {
            if (dynamic_pointer_cast<IntegerLiteral>(declaration->initializer))
            {
                type = std::make_shared<IntegerType>(TypeQualifier::Constant);
            }
            else if (dynamic_pointer_cast<StringLiteral>(declaration->initializer))
            {
                type = std::make_shared<StringType>(TypeQualifier::Constant);
            }
            else if (auto function = dynamic_pointer_cast<Function>(declaration->initializer))
            {
                auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
                for (auto& parameter : *function->parameters)
                {
                    parameters->push_back(converter_.Convert(*parameter->annotation));
                }

                auto return_type = converter_.Convert(*function->return_type_annotation);

                type = std::make_shared<FunctionType>(parameters, return_type, TypeQualifier::Constant);
            }
        }

        module_.globals->Declare(declaration->variable, type);
        declaration->scope = module_.globals;
    }
}

}  // namespace l0
