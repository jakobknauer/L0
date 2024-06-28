#include "l0/generation/type_converter.h"

#include "l0/generation/generator_error.h"

namespace l0
{

TypeConverter::TypeConverter(llvm::LLVMContext& context) : context_{context}
{
    llvm::StructType::create(context_, {}, "Unit", true);
}

llvm::Type* TypeConverter::Convert(const Type& type)
{
    if (dynamic_cast<const StringType*>(&type))
    {
        return llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0);
    }
    else if (dynamic_cast<const IntegerType*>(&type))
    {
        return llvm::IntegerType::getInt64Ty(context_);
    }
    else if (dynamic_cast<const BooleanType*>(&type))
    {
        return llvm::IntegerType::getInt1Ty(context_);
    }
    else if (dynamic_cast<const UnitType*>(&type))
    {
        return llvm::StructType::getTypeByName(context_, "Unit");
    }
    else if (dynamic_cast<const FunctionType*>(&type))
    {
        return Convert(dynamic_cast<const FunctionType&>(type));
    }
    else
    {
        throw GeneratorError(std::format("Cannot convert l0 type {} to llvm type.", type.ToString()));
    }
}

llvm::FunctionType* TypeConverter::Convert(const FunctionType& type)
{
    std::vector<llvm::Type*> params;
    for (const auto& param : *type.parameters)
    {
        params.push_back(Convert(*param));
    }

    auto return_type = Convert(*type.return_type);

    return llvm::FunctionType::get(return_type, params, false);
}

llvm::FunctionType* TypeConverter::GetDeclarationType(const FunctionType& type)
{
    std::vector<llvm::Type*> params;
    for (const auto& param : *type.parameters)
    {
        if (dynamic_cast<const FunctionType*>(param.get()))
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

}  // namespace l0
