#ifndef L0_GENERATION_TYPE_CONVERTER
#define L0_GENERATION_TYPE_CONVERTER

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include "l0/types/types.h"

namespace l0
{

class TypeConverter : private IConstTypeVisitor
{
   public:
    TypeConverter(llvm::LLVMContext& context);

    llvm::Type* Convert(const Type& type);
    llvm::FunctionType* Convert(const FunctionType& type);
    llvm::FunctionType* GetFunctionDeclarationType(const FunctionType& type);
    llvm::Type* GetValueDeclarationType(const Type& type);

   private:
    void Visit(const ReferenceType& reference_type) override;
    void Visit(const UnitType& unit_type) override;
    void Visit(const BooleanType& boolean_type) override;
    void Visit(const IntegerType& integer_type) override;
    void Visit(const CharacterType& character_type) override;
    void Visit(const FunctionType& function_type) override;
    void Visit(const StructType& struct_type) override;
    void Visit(const EnumType& enum_type) override;

    llvm::LLVMContext& context_;
    llvm::Type* result_;
};

}  // namespace l0

#endif
