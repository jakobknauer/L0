#include "l0/generation/generation.h"

#include "l0/generation/generator.h"

namespace l0
{

void GenerateIR(Module& module, llvm::LLVMContext& context)
{
    detail::Generator{context, module}.Run();
}

}  // namespace l0
