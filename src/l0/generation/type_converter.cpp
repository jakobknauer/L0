#include "l0/generation/type_converter.h"

namespace l0
{

TypeConverter::TypeConverter(llvm::LLVMContext& context) : context_{context}
{
    llvm::StructType::create(context_, {}, "Unit", true);
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

llvm::FunctionType* TypeConverter::GetDeclarationType(const FunctionType& type)
{
    std::vector<llvm::Type*> params;
    for (const auto& param : *type.parameters)
    {
        if (dynamic_pointer_cast<FunctionType>(param))
        {
            params.push_back(llvm::PointerType::getUnqual(context_));
        }
        else
        {
            params.push_back(Convert(*param));
        }
    }

    llvm::Type* return_type;
    if (dynamic_cast<const FunctionType*>(type.return_type.get()))
    {
        return_type = llvm::PointerType::getUnqual(context_);
    }
    else
    {
        return_type = Convert(*type.return_type);
    }

    return llvm::FunctionType::get(return_type, params, false);
}

void TypeConverter::Visit(const ReferenceType& reference_type)
{
    reference_type.base_type->Accept(*this);
    result_ = llvm::PointerType::get(result_, 0);
}

void TypeConverter::Visit(const UnitType& unit_type) { result_ = llvm::StructType::getTypeByName(context_, "Unit"); }

void TypeConverter::Visit(const BooleanType& boolean_type) { result_ = llvm::IntegerType::getInt1Ty(context_); }

void TypeConverter::Visit(const IntegerType& integer_type) { result_ = llvm::IntegerType::getInt64Ty(context_); }

void TypeConverter::Visit(const StringType& string_type)
{
    result_ = llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0);
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

}  // namespace l0
