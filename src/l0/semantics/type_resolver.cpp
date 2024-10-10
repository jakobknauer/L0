#include "l0/semantics/type_resolver.h"

#include "l0/semantics/semantic_error.h"

namespace l0::detail
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

std::shared_ptr<Type> TypeResolver::Resolve(std::string_view name)
{
    if (module_.externals->IsTypeDeclared(name))
    {
        return module_.externals->GetTypeDefinition(name);
    }
    else if (module_.globals->IsTypeDeclared(name))
    {
        return module_.globals->GetTypeDefinition(name);
    }
    else
    {
        throw SemanticError(std::format("Cannot resolve type name '{}'.", name));
    }
}

void TypeResolver::Visit(const SimpleTypeAnnotation& sta)
{
    auto type = Resolve(sta.type);
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

void TypeResolver::Visit(const MutabilityOnlyTypeAnnotation& mota)
{
    throw SemanticError(std::format("Unexpected mutability-only type annotation."));
}

}  // namespace l0::detail
