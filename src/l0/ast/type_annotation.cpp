#include "l0/ast/type_annotation.h"

namespace l0
{

SimpleTypeAnnotation::SimpleTypeAnnotation(Identifier type_name)
    : type_name{type_name}
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

MethodTypeAnnotation::MethodTypeAnnotation(std::shared_ptr<FunctionTypeAnnotation> function_type)
    : function_type{function_type}
{
}

void MethodTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const
{
    visitor.Visit(*this);
};

void MutabilityOnlyTypeAnnotation::Accept(ITypeAnnotationVisitor& visitor) const
{
    visitor.Visit(*this);
};

}  // namespace l0
