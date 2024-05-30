#ifndef L0_AST_TYPE_ANNOTATION_H
#define L0_AST_TYPE_ANNOTATION_H

#include <memory>
#include <string>
#include <vector>

namespace l0
{

class ITypeAnnotationVisitor;

class TypeAnnotation
{
   public:
    virtual ~TypeAnnotation() = default;

    virtual void Accept(ITypeAnnotationVisitor& visitor) const = 0;
};

class SimpleTypeAnnotation : public TypeAnnotation
{
   public:
    SimpleTypeAnnotation(std::string type);

    void Accept(ITypeAnnotationVisitor& visitor) const override;

    std::string type;
};

using ParameterListAnnotation = std::vector<std::unique_ptr<TypeAnnotation>>;

class FunctionTypeAnnotation : public TypeAnnotation
{
   public:
    FunctionTypeAnnotation(
        std::unique_ptr<ParameterListAnnotation> parameters, std::unique_ptr<TypeAnnotation> return_type
    );

    void Accept(ITypeAnnotationVisitor& visitor) const override;

    std::unique_ptr<ParameterListAnnotation> parameters;
    std::unique_ptr<TypeAnnotation> return_type;
};

class ITypeAnnotationVisitor
{
   public:
    virtual ~ITypeAnnotationVisitor() = default;

    virtual void Visit(const SimpleTypeAnnotation& sta) = 0;
    virtual void Visit(const FunctionTypeAnnotation& fta) = 0;
};

}  // namespace l0

#endif
