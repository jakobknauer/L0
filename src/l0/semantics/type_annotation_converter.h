#ifndef L0_SEMANTICS_TYPE_ANNOTATION_CONVERTER_H
#define L0_SEMANTICS_TYPE_ANNOTATION_CONVERTER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "l0/ast/type_annotation.h"
#include "l0/types/types.h"

namespace l0
{

class TypeAnnotationConverter : private ITypeAnnotationVisitor
{
   public:
    TypeAnnotationConverter();
    std::shared_ptr<Type> Convert(const TypeAnnotation& annotation);

   private:
    std::shared_ptr<Type> result_;

    std::unordered_map<std::string, const std::shared_ptr<Type>> simple_types_;

    void Visit(const SimpleTypeAnnotation& sta) override;
    void Visit(const ReferenceTypeAnnotation& rta) override;
    void Visit(const FunctionTypeAnnotation& fta) override;
};

}  // namespace l0

#endif
