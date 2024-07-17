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
        auto declaration = dynamic_pointer_cast<Declaration>(statement);

        if (!declaration)
        {
            throw SemanticError("Only declarations are allowed as global statements.");
        }

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
    }
}

};  // namespace l0
