#ifndef L0_SEMANTICS_CONVERSION_CHECKER_H
#define L0_SEMANTICS_CONVERSION_CHECKER_H

#include <memory>

#include "l0/types/types.h"

namespace l0::detail
{

class ConversionChecker : IConstTypeVisitor
{
   public:
    bool CheckCompatibility(std::shared_ptr<Type> target, std::shared_ptr<Type> value);

   private:
    bool result_;
    std::shared_ptr<Type> value_;

    void Visit(const ReferenceType& reference_type) override;
    void Visit(const UnitType& unit_type) override;
    void Visit(const BooleanType& boolean_type) override;
    void Visit(const IntegerType& integer_type) override;
    void Visit(const CharacterType& character_type) override;
    void Visit(const FunctionType& function_type) override;
    void Visit(const StructType& struct_type) override;
};

}  // namespace l0::detail

#endif
