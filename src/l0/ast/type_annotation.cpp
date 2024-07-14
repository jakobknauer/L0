#include "l0/ast/type_annotation.h"

namespace l0
{

SimpleTypeAnnotation::SimpleTypeAnnotation(std::string type) : type{type} {}

void SimpleTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const { visitor.Visit(*this); }

ReferenceTypeAnnotation::ReferenceTypeAnnotation(std::unique_ptr<TypeAnnotation> base_type)
    : base_type{std::move(base_type)}
{
}

void ReferenceTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const { visitor.Visit(*this); }

FunctionTypeAnnotation::FunctionTypeAnnotation(
    std::unique_ptr<ParameterListAnnotation> parameters, std::unique_ptr<TypeAnnotation> return_type
)
    : parameters{std::move(parameters)}, return_type{std::move(return_type)}
{
}

void FunctionTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const { visitor.Visit(*this); };

}  // namespace l0
