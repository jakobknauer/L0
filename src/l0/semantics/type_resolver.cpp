#include "l0/semantics/type_resolver.h"

#include <utility>

#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

TypeResolver::TypeResolver(const Module& module)
    : module_{module}
{
}

std::shared_ptr<Type> TypeResolver::Convert(const TypeAnnotation& annotation, Identifier namespace_)
{
    this->namespace_ = namespace_;
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
    std::unreachable();
}

std::shared_ptr<FunctionType> TypeResolver::Convert(const Function& function, Identifier namespace_)
{
    auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
    for (auto& parameter : *function.parameters)
    {
        parameters->push_back(Convert(*parameter->annotation, namespace_));
    }

    auto return_type = Convert(*function.return_type_annotation, namespace_);

    return std::make_shared<FunctionType>(parameters, return_type, TypeQualifier::Constant);
}

std::shared_ptr<Scope> TypeResolver::Resolve(const Identifier& identifier, [[maybe_unused]] Identifier namespace_)
{
    auto global_resolution = Resolve(identifier);
    if (global_resolution)
    {
        return *global_resolution;
    }

    auto resolution_in_namespace = Resolve(namespace_ + identifier);
    if (resolution_in_namespace)
    {
        return *resolution_in_namespace;
    }

    throw SemanticError(std::format("Cannot resolve type name '{}'.", identifier.ToString()));
}

std::optional<std::shared_ptr<Scope>> TypeResolver::Resolve(const Identifier& identifier)
{
    if (module_.environment->IsTypeDeclared(identifier))
    {
        return module_.environment;
    }
    if (module_.externals->IsTypeDeclared(identifier))
    {
        return module_.externals;
    }
    else if (module_.globals->IsTypeDeclared(identifier))
    {
        return module_.globals;
    }
    else
    {
        return std::nullopt;
    }
}

std::shared_ptr<Type> TypeResolver::GetTypeByName(const Identifier& identifier, Identifier namespace_)
{
    return Resolve(identifier, namespace_)->GetTypeDefinition(identifier);
}

void TypeResolver::Visit(const SimpleTypeAnnotation& sta)
{
    auto type = GetTypeByName(sta.type_name, namespace_);
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

void TypeResolver::Visit(const MethodTypeAnnotation&)
{
    throw SemanticError(std::format("Unexpected method type annotation."));
}

void TypeResolver::Visit(const MutabilityOnlyTypeAnnotation&)
{
    throw SemanticError(std::format("Unexpected mutability-only type annotation."));
}

}  // namespace l0::detail
