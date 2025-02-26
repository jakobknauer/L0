#include "l0/generation/type_converter.h"

#include "l0/common/constants.h"

namespace l0
{

TypeConverter::TypeConverter(llvm::LLVMContext& context)
    : context_{context}
{
    llvm::StructType::create(context_, {}, Typename::Unit, true);
}

llvm::Type* TypeConverter::Convert(const Type& type)
{
    type.Accept(*this);
    return result_;
}

llvm::FunctionType* TypeConverter::Convert(const FunctionType& type)
{
    type.Accept(*this);
    return llvm::dyn_cast<llvm::FunctionType>(result_);
}

llvm::FunctionType* TypeConverter::GetFunctionDeclarationType(const FunctionType& type)
{
    std::vector<llvm::Type*> params;
    for (const auto& param : *type.parameters)
    {
        params.push_back(GetValueDeclarationType(*param));
    }
    params.push_back(llvm::PointerType::get(context_, 0));
    llvm::Type* return_type = GetValueDeclarationType(*type.return_type);
    return llvm::FunctionType::get(return_type, params, false);
}

llvm::Type* TypeConverter::GetValueDeclarationType(const Type& type)
{
    if (dynamic_cast<const FunctionType*>(&type))
    {
        return llvm::StructType::getTypeByName(context_, "__closure");
    }
    else
    {
        return Convert(type);
    }
}

void TypeConverter::Visit(const ReferenceType& reference_type)
{
    reference_type.base_type->Accept(*this);
    result_ = llvm::PointerType::get(result_, 0);
}

void TypeConverter::Visit(const UnitType&)
{
    result_ = llvm::StructType::getTypeByName(context_, Typename::Unit);
}

void TypeConverter::Visit(const BooleanType&)
{
    result_ = llvm::IntegerType::getInt1Ty(context_);
}

void TypeConverter::Visit(const IntegerType&)
{
    result_ = llvm::IntegerType::getInt64Ty(context_);
}

void TypeConverter::Visit(const CharacterType&)
{
    result_ = llvm::IntegerType::getInt8Ty(context_);
}

void TypeConverter::Visit(const FunctionType& function_type)
{
    std::vector<llvm::Type*> params;
    for (const auto& param : *function_type.parameters)
    {
        param->Accept(*this);
        params.push_back(result_);
    }

    function_type.return_type->Accept(*this);
    auto return_type = result_;

    result_ = llvm::FunctionType::get(return_type, params, false);
}

void TypeConverter::Visit(const StructType& struct_type)
{
    auto llvm_struct_type = llvm::StructType::getTypeByName(context_, struct_type.identifier.ToString());
    result_ = llvm_struct_type;
}

void TypeConverter::Visit(const EnumType&)
{
    result_ = llvm::IntegerType::getInt64Ty(context_);
}

}  // namespace l0
