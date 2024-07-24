#include "l0/semantics/type_annotation_converter.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

std::shared_ptr<Type> TypeAnnotationConverter::Convert(const TypeAnnotation& annotation)
{
    annotation.Accept(*this);
    return result_;
}

TypeQualifier TypeAnnotationConverter::Convert(TypeAnnotationQualifier qualifier)
{
    switch (qualifier)
    {
        case TypeAnnotationQualifier::None:
        case TypeAnnotationQualifier::Constant:
            return TypeQualifier::Constant;
        case l0::TypeAnnotationQualifier::Mutable:
            return TypeQualifier::Mutable;
    }
}

void TypeAnnotationConverter::Visit(const SimpleTypeAnnotation& sta)
{
    auto mutability = Convert(sta.mutability);

    if (sta.type == "()")
    {
        result_ = std::make_shared<UnitType>(mutability);
    }
    else if (sta.type == "Boolean")
    {
        result_ = std::make_shared<BooleanType>(mutability);
    }
    else if (sta.type == "Integer")
    {
        result_ = std::make_shared<IntegerType>(mutability);
    }
    else if (sta.type == "String")
    {
        result_ = std::make_shared<StringType>(mutability);
    }
    else
    {
        throw SemanticError(std::format("Unknown simple type '{}'.", sta.type));
    }
}

void TypeAnnotationConverter::Visit(const ReferenceTypeAnnotation& rta)
{
    rta.base_type->Accept(*this);
    auto base_type = result_;
    auto mutability = Convert(rta.mutability);
    auto reference_type = std::make_shared<ReferenceType>(base_type, mutability);
    result_ = reference_type;
}

void TypeAnnotationConverter::Visit(const FunctionTypeAnnotation& fta)
{
    fta.return_type->Accept(*this);
    auto return_type = result_;

    auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
    for (auto& parameter : *fta.parameters)
    {
        parameter->Accept(*this);
        parameters->push_back(result_);
    }

    auto mutability = Convert(fta.mutability);
    result_ = std::make_shared<FunctionType>(parameters, return_type, mutability);
}

}  // namespace l0
