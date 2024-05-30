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
        auto declaration = dynamic_cast<Declaration*>(statement.get());

        if (module_.globals->IsDeclared(declaration->variable))
        {
            throw SemanticError(std::format("Duplicate declaration of global variable '{}'.", declaration->variable));
        }

        auto type = converter_.Convert(*declaration->annotation);

        module_.globals->Declare(declaration->variable, type);
        declaration->scope = module_.globals;
    }
}

}  // namespace l0
