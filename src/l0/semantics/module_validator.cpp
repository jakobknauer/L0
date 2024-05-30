#include "l0/semantics/module_validator.h"

#include "l0/ast/statement.h"
#include "l0/semantics/semantic_error.h"

namespace l0
{

ModuleValidator::ModuleValidator(const Module& module) : module_{module} {}

void ModuleValidator::Check() const
{
    for (auto& statement : *module_.statements)
    {
        auto declaration = dynamic_cast<Declaration*>(statement.get());

        if (!declaration)
        {
            throw SemanticError("Only declarations are allowed as global statements.");
        }

        auto initializer = declaration->initializer.get();
        if (!dynamic_cast<IntegerLiteral*>(initializer) && !dynamic_cast<StringLiteral*>(initializer) &&
            !dynamic_cast<Function*>(initializer))
        {
            throw SemanticError(
                std::format("Initializer of global variable must be a literal or function.", declaration->variable)
            );
        }
    }
}

};  // namespace l0
