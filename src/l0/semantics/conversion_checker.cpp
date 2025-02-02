#include "l0/semantics/conversion_checker.h"

#include <ranges>

#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

ConversionChecker::ConversionChecker(TypeResolver& resolver)
    : resolver_{resolver}
{
}

bool ConversionChecker::CheckCompatibility(std::shared_ptr<Type> target, std::shared_ptr<Type> value)
{
    value_ = value;
    target->Accept(*this);
    return result_;
}

std::shared_ptr<Type> ConversionChecker::Coerce(
    std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<Type> actual
)
{
    if (!annotation)
    {
        return ModifyQualifier(*actual, TypeQualifier::Constant);
    }

    if (dynamic_pointer_cast<MutabilityOnlyTypeAnnotation>(annotation))
    {
        switch (annotation->mutability)
        {
            case TypeAnnotationQualifier::Mutable:
                return ModifyQualifier(*actual, TypeQualifier::Mutable);
            case TypeAnnotationQualifier::Constant:
                return ModifyQualifier(*actual, TypeQualifier::Constant);
            case TypeAnnotationQualifier::None:
                throw SemanticError("MutabilityOnlyTypeAnnotation cannot have 'None' mutability.");
        }
    }

    auto annotated_type = resolver_.Convert(*annotation);
    if (CheckCompatibility(annotated_type, actual))
    {
        return annotated_type;
    }

    return nullptr;
}

void ConversionChecker::Visit(const ReferenceType& reference_type)
{
    auto value_as_reference_type = dynamic_pointer_cast<ReferenceType>(value_);
    if (!value_as_reference_type)
    {
        result_ = false;
        return;
    }

    if (reference_type.base_type->mutability == TypeQualifier::Mutable
        && value_as_reference_type->base_type->mutability == TypeQualifier::Constant)
    {
        result_ = false;
        return;
    }

    value_ = value_as_reference_type->base_type;
    reference_type.base_type->Accept(*this);
    // leave result_ as is
}

void ConversionChecker::Visit(const UnitType&)
{
    result_ = bool{dynamic_pointer_cast<UnitType>(value_)};
}

void ConversionChecker::Visit(const BooleanType&)
{
    result_ = bool{dynamic_pointer_cast<BooleanType>(value_)};
}

void ConversionChecker::Visit(const IntegerType&)
{
    result_ = dynamic_pointer_cast<IntegerType>(value_) != nullptr;
}

void ConversionChecker::Visit(const CharacterType&)
{
    result_ = dynamic_pointer_cast<CharacterType>(value_) != nullptr;
}

void ConversionChecker::Visit(const FunctionType& function_type)
{
    auto value_as_function_type = dynamic_pointer_cast<FunctionType>(value_);
    if (!value_as_function_type)
    {
        result_ = false;
        return;
    }

    if (function_type.parameters->size() != value_as_function_type->parameters->size())
    {
        result_ = false;
        return;
    }

    for (auto [parameter_type, argument_type] :
         std::ranges::zip_view(*function_type.parameters, *value_as_function_type->parameters))
    {
        value_ = parameter_type;
        argument_type->Accept(*this);
        if (!result_)
        {
            // leave result_ false
            return;
        }
    }

    value_ = value_as_function_type->return_type;
    function_type.return_type->Accept(*this);
    // leave result_ as is
}

void ConversionChecker::Visit(const StructType& struct_type)
{
    auto value_as_struct_type = dynamic_pointer_cast<StructType>(value_);
    result_ = value_as_struct_type && (struct_type.name == value_as_struct_type->name);
}

void ConversionChecker::Visit(const EnumType& enum_type)
{
    auto value_as_struct_type = dynamic_pointer_cast<EnumType>(value_);
    result_ = value_as_struct_type && (enum_type.name == value_as_struct_type->name);
}

}  // namespace l0::detail
