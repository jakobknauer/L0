#ifndef L0_GENERATION_GENERATION_H
#define L0_GENERATION_GENERATION_H

#include <llvm/IR/LLVMContext.h>

#include "l0/ast/module.h"

namespace l0
{

void GenerateIR(Module& module, llvm::LLVMContext& context);

}

#endif
