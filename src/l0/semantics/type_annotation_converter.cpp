#include "l0/semantics/type_annotation_converter.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

TypeAnnotationConverter::TypeAnnotationConverter()
{
    simple_types_.insert(std::make_pair("()", std::make_shared<UnitType>()));
    simple_types_.insert(std::make_pair("Boolean", std::make_shared<BooleanType>()));
    simple_types_.insert(std::make_pair("Integer", std::make_shared<IntegerType>()));
    simple_types_.insert(std::make_pair("String", std::make_shared<StringType>()));
}

std::shared_ptr<Type> TypeAnnotationConverter::Convert(const TypeAnnotation& annotation)
{
    annotation.Accept(*this);
    return result_;
}

void TypeAnnotationConverter::Visit(const SimpleTypeAnnotation& sta)
{
    auto it = simple_types_.find(sta.type);
    if (it != simple_types_.end())
    {
        result_ = simple_types_.at(sta.type);
    }
    else
    {
        throw SemanticError(std::format("Unknown simple type '{}'.", sta.type));
    }
}

void TypeAnnotationConverter::Visit(const ReferenceTypeAnnotation& rta)
{
    rta.base_type->Accept(*this);
    auto reference_type = std::make_shared<ReferenceType>();
    reference_type->base_type = result_;
    result_ = reference_type;
}

void TypeAnnotationConverter::Visit(const FunctionTypeAnnotation& fta)
{
    auto type = std::make_shared<FunctionType>();

    fta.return_type->Accept(*this);
    type->return_type = result_;

    type->parameters = std::make_shared<ParameterList>();
    for (auto& parameter : *fta.parameters)
    {
        parameter->Accept(*this);
        type->parameters->push_back(result_);
    }

    result_ = type;
}

}  // namespace l0
