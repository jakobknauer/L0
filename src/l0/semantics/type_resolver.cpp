#include "l0/semantics/type_resolver.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

TypeResolver::TypeResolver(const Module& module)
    : module_{module}
{
}

std::shared_ptr<Type> TypeResolver::Convert(const TypeAnnotation& annotation)
{
    annotation.Accept(*this);
    return result_;
}

TypeQualifier TypeResolver::Convert(TypeAnnotationQualifier qualifier)
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

std::shared_ptr<FunctionType> TypeResolver::Convert(const Function& function)
{
    auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
    for (auto& parameter : *function.parameters)
    {
        parameters->push_back(Convert(*parameter->annotation));
    }

    auto return_type = Convert(*function.return_type_annotation);

    return std::make_shared<FunctionType>(parameters, return_type, TypeQualifier::Constant);
}

void TypeResolver::Visit(const SimpleTypeAnnotation& sta)
{
    if (!module_.globals->IsTypeDeclared(sta.type))
    {
        throw SemanticError(std::format("Unknown simple type '{}'.", sta.type));
    }

    auto type = module_.globals->GetTypeDefinition(sta.type);
    auto mutability = Convert(sta.mutability);
    result_ = ModifyQualifier(*type, mutability);
}

void TypeResolver::Visit(const ReferenceTypeAnnotation& rta)
{
    rta.base_type->Accept(*this);
    auto base_type = result_;
    auto mutability = Convert(rta.mutability);
    auto reference_type = std::make_shared<ReferenceType>(base_type, mutability);
    result_ = reference_type;
}

void TypeResolver::Visit(const FunctionTypeAnnotation& fta)
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

void TypeResolver::Visit(const MethodTypeAnnotation& mta)
{
    throw SemanticError(std::format("Unexpected method type annotation."));
}

}  // namespace l0
