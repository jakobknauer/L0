#ifndef L0_SEMANTICS_BINARY_OP_OVERLOAD_RESOLVER_H
#define L0_SEMANTICS_BINARY_OP_OVERLOAD_RESOLVER_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "l0/ast/expression.h"
#include "l0/types/types.h"

namespace l0::detail
{

class OperatorOverloadResolver
{
   public:
    OperatorOverloadResolver();
    std::shared_ptr<Type> ResolveUnaryOperator(UnaryOp::Operator op, std::shared_ptr<Type> operand) const;
    std::shared_ptr<Type> ResolveBinaryOperator(
        BinaryOp::Operator op, std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs
    ) const;

   private:
    struct UnaryOpSignature
    {
        std::shared_ptr<Type> operand;
        std::shared_ptr<Type> result;
    };

    struct BinaryOpSignature
    {
        std::shared_ptr<Type> lhs;
        std::shared_ptr<Type> rhs;
        std::shared_ptr<Type> result;
    };

    std::unordered_map<UnaryOp::Operator, std::vector<UnaryOpSignature>> unary_operator_overloads_;
    std::unordered_map<BinaryOp::Operator, std::vector<BinaryOpSignature>> binary_operator_overloads_;
};

}  // namespace l0::detail

#endif
