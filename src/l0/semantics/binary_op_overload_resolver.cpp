#include "l0/semantics/binary_op_overload_resolver.h"

#include "l0/ast/ast_printer.h"

namespace l0::detail
{

BinaryOpOverloadResolver::BinaryOpOverloadResolver()
{
    auto boolean = std::make_shared<BooleanType>();
    auto integer = std::make_shared<IntegerType>();
    auto string = std::make_shared<StringType>();

    overloads_ = {
        {BinaryOp::Operator::EqualsEquals, {{boolean, boolean, boolean}, {integer, integer, boolean}}},
        {BinaryOp::Operator::BangEquals, {{boolean, boolean, boolean}, {integer, integer, boolean}}},

        {BinaryOp::Operator::Plus, {{integer, integer, integer}, {string, string, string}}},
        {BinaryOp::Operator::Minus, {{integer, integer, integer}}},
        {BinaryOp::Operator::Asterisk, {{integer, integer, integer}}},

        {BinaryOp::Operator::PipePipe, {{boolean, boolean, boolean}}},
        {BinaryOp::Operator::AmpersandAmpersand, {{boolean, boolean, boolean}}},
    };
}

std::shared_ptr<Type> BinaryOpOverloadResolver::Resolve(BinaryOp::Operator op, const Type& lhs, const Type& rhs)
    const
{
    auto& candidates = overloads_.at(op);
    auto hit = std::find_if(
        candidates.cbegin(),
        candidates.cend(),
        [&](const BinaryOpSignature& signature) { return *signature.lhs == lhs && *signature.rhs == rhs; }
    );

    if (hit == candidates.cend())
    {
        throw SemanticError(std::format(
            "No viable overload of operator '{}' with left-hand side of type '{}' and right-hand side of type '{}'.",
            str(op),
            lhs.ToString(),
            rhs.ToString()
        ));
    }

    return hit->result;
}

}  // namespace l0::detail
