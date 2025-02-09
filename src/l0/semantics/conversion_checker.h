#ifndef L0_SEMANTICS_CONVERSION_CHECKER_H
#define L0_SEMANTICS_CONVERSION_CHECKER_H

#include <memory>

#include "l0/ast/type_annotation.h"
#include "l0/semantics/type_resolver.h"
#include "l0/types/types.h"

namespace l0::detail
{

class ConversionChecker : IConstTypeVisitor
{
   public:
    ConversionChecker(TypeResolver& resolver);
    bool CheckCompatibility(std::shared_ptr<Type> target, std::shared_ptr<Type> value);
    std::shared_ptr<Type> Coerce(std::shared_ptr<TypeAnnotation> annotation, std::shared_ptr<Type> actual, Identifier namespace_);

   private:
    TypeResolver& resolver_;
    bool result_;
    std::shared_ptr<Type> value_;

    void Visit(const ReferenceType& reference_type) override;
    void Visit(const UnitType& unit_type) override;
    void Visit(const BooleanType& boolean_type) override;
    void Visit(const IntegerType& integer_type) override;
    void Visit(const CharacterType& character_type) override;
    void Visit(const FunctionType& function_type) override;
    void Visit(const StructType& struct_type) override;
    void Visit(const EnumType& enum_type) override;
};

}  // namespace l0::detail

#endif
