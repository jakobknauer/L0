#ifndef L0_SEMANTICS_BINARY_OP_OVERLOAD_RESOLVER_H
#define L0_SEMANTICS_BINARY_OP_OVERLOAD_RESOLVER_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "l0/ast/expression.h"
#include "l0/semantics/semantic_error.h"
#include "l0/types/types.h"

namespace l0::detail
{

class BinaryOpOverloadResolver
{
   public:
    BinaryOpOverloadResolver();
    std::shared_ptr<Type> Resolve(BinaryOp::Operator op, const Type& lhs, const Type& rhs) const;

   private:
    struct BinaryOpSignature
    {
        std::shared_ptr<Type> lhs;
        std::shared_ptr<Type> rhs;
        std::shared_ptr<Type> result;
    };

    std::unordered_map<BinaryOp::Operator, std::vector<BinaryOpSignature>> overloads_;
};

}  // namespace l0::detail

#endif
