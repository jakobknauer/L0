#include "l0/ast/type_annotation.h"

namespace l0
{

SimpleTypeAnnotation::SimpleTypeAnnotation(std::string type)
    : type{type}
{
}

void SimpleTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const
{
    visitor.Visit(*this);
}

ReferenceTypeAnnotation::ReferenceTypeAnnotation(std::shared_ptr<TypeAnnotation> base_type)
    : base_type{base_type}
{
}

void ReferenceTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const
{
    visitor.Visit(*this);
}

FunctionTypeAnnotation::FunctionTypeAnnotation(
    std::shared_ptr<ParameterListAnnotation> parameters, std::shared_ptr<TypeAnnotation> return_type
)
    : parameters{parameters},
      return_type{return_type}
{
}

void FunctionTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const
{
    visitor.Visit(*this);
};

}  // namespace l0
