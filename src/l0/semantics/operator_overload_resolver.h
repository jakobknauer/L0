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

    struct UnaryOpResolution
    {
        std::shared_ptr<Type> result_type;
        UnaryOp::Overload overload;
    };
    UnaryOpResolution ResolveUnaryOperator(UnaryOp::Operator op, std::shared_ptr<Type> operand) const;

    struct BinaryOpResolution
    {
        std::shared_ptr<Type> result_type;
        BinaryOp::Overload overload;
    };
    BinaryOpResolution ResolveBinaryOperator(
        BinaryOp::Operator op, std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs
    ) const;

   private:
    struct UnaryOpSignature
    {
        std::shared_ptr<Type> operand;
        UnaryOpResolution resolution;
    };

    struct BinaryOpSignature
    {
        std::shared_ptr<Type> lhs;
        std::shared_ptr<Type> rhs;
        BinaryOpResolution resolution;
    };

    std::unordered_map<UnaryOp::Operator, std::vector<UnaryOpSignature>> unary_operator_overloads_;
    std::unordered_map<BinaryOp::Operator, std::vector<BinaryOpSignature>> binary_operator_overloads_;
};

}  // namespace l0::detail

#endif
