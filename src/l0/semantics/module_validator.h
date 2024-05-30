#ifndef L0_SEMANTICS_MODULE_VALIDATOR_H
#define L0_SEMANTICS_MODULE_VALIDATOR_H

#include "l0/ast/module.h"

namespace l0
{

// Checks validity of module level statements
//
// This includes:
// - Only declarations of immutable variables are allowed as module level statements.
// - Initializers may only be literals or functions.
class ModuleValidator
{
   public:
    ModuleValidator(const Module& module);

    void Check() const;

   private:
    const Module& module_;
};

}  // namespace l0

#endif
