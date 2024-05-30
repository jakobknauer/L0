#ifndef L0_GENERATION_TYPE_CONVERTER
#define L0_GENERATION_TYPE_CONVERTER

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include "l0/types/types.h"

namespace l0
{

class TypeConverter
{
   public:
    TypeConverter(llvm::LLVMContext& context);
    llvm::Type* Convert(const Type& type);
    llvm::FunctionType* Convert(const FunctionType& type);
    llvm::FunctionType* GetDeclarationType(const FunctionType& type);

   private:
    llvm::LLVMContext& context_;
};

}  // namespace l0

#endif
