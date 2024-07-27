#ifndef L0_AST_TYPE_ANNOTATION_H
#define L0_AST_TYPE_ANNOTATION_H

#include <memory>
#include <string>
#include <vector>

namespace l0
{

class ITypeAnnotationVisitor;

enum class TypeAnnotationQualifier
{
    None,
    Constant,
    Mutable,
};

class TypeAnnotation
{
   public:
    virtual ~TypeAnnotation() = default;

    virtual void Accept(ITypeAnnotationVisitor& visitor) const = 0;

    TypeAnnotationQualifier mutability{TypeAnnotationQualifier::None};
};

class SimpleTypeAnnotation : public TypeAnnotation
{
   public:
    SimpleTypeAnnotation(std::string type);

    void Accept(ITypeAnnotationVisitor& visitor) const override;

    std::string type;
};

class ReferenceTypeAnnotation : public TypeAnnotation
{
   public:
    ReferenceTypeAnnotation(std::shared_ptr<TypeAnnotation> base_type);

    void Accept(ITypeAnnotationVisitor& visitor) const override;

    std::shared_ptr<TypeAnnotation> base_type;
};

using ParameterListAnnotation = std::vector<std::shared_ptr<TypeAnnotation>>;

class FunctionTypeAnnotation : public TypeAnnotation
{
   public:
    FunctionTypeAnnotation(
        std::shared_ptr<ParameterListAnnotation> parameters, std::shared_ptr<TypeAnnotation> return_type
    );

    void Accept(ITypeAnnotationVisitor& visitor) const override;

    std::shared_ptr<ParameterListAnnotation> parameters;
    std::shared_ptr<TypeAnnotation> return_type;
};

class ITypeAnnotationVisitor
{
   public:
    virtual ~ITypeAnnotationVisitor() = default;

    virtual void Visit(const SimpleTypeAnnotation& sta) = 0;
    virtual void Visit(const ReferenceTypeAnnotation& rta) = 0;
    virtual void Visit(const FunctionTypeAnnotation& fta) = 0;
};

}  // namespace l0

#endif
