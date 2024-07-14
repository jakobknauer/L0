#include "l0/semantics/operator_overload_resolver.h"

#include "l0/ast/ast_printer.h"
#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

OperatorOverloadResolver::OperatorOverloadResolver()
{
    auto boolean = std::make_shared<BooleanType>();
    auto integer = std::make_shared<IntegerType>();
    auto string = std::make_shared<StringType>();

    unary_operator_overloads_ = {
        {UnaryOp::Operator::Plus, {{integer, integer}}},
        {UnaryOp::Operator::Minus, {{integer, integer}}},
        {UnaryOp::Operator::Bang, {{boolean, boolean}}},
    };

    binary_operator_overloads_ = {
        {BinaryOp::Operator::EqualsEquals, {{boolean, boolean, boolean}, {integer, integer, boolean}}},
        {BinaryOp::Operator::BangEquals, {{boolean, boolean, boolean}, {integer, integer, boolean}}},

        {BinaryOp::Operator::Plus, {{integer, integer, integer}, {string, string, string}}},
        {BinaryOp::Operator::Minus, {{integer, integer, integer}}},
        {BinaryOp::Operator::Asterisk, {{integer, integer, integer}}},

        {BinaryOp::Operator::PipePipe, {{boolean, boolean, boolean}}},
        {BinaryOp::Operator::AmpersandAmpersand, {{boolean, boolean, boolean}}},
    };
}

std::shared_ptr<Type> OperatorOverloadResolver::ResolveUnaryOperator(
    UnaryOp::Operator op, std::shared_ptr<Type> operand
) const
{
    if (op == UnaryOp::Operator::Ampersand)
    {
        auto reference_type = std::make_shared<ReferenceType>();
        reference_type->base_type = operand;
        return reference_type;
    }
    if (op == UnaryOp::Operator::Asterisk)
    {
        auto reference_type = dynamic_cast<ReferenceType*>(operand.get());
        if (!reference_type)
        {
            throw SemanticError(std::format("Cannot dereference value of type '{}'.", operand->ToString()));
        }
        return reference_type->base_type;
    }

    if (!unary_operator_overloads_.contains(op))
    {
        throw SemanticError(std::format("No known overloads of unary operator '{}'.", str(op)));
    }

    auto& candidates = unary_operator_overloads_.at(op);
    auto matching_signature = std::find_if(
        candidates.cbegin(),
        candidates.cend(),
        [&](const UnaryOpSignature& signature) { return *signature.operand == *operand; }
    );

    if (matching_signature != candidates.cend())
    {
        return matching_signature->result;
    }

    throw SemanticError(std::format(
        "No viable overload of unary operator '{}' with operand of type '{}'.", str(op), operand->ToString()
    ));
}

std::shared_ptr<Type> OperatorOverloadResolver::ResolveBinaryOperator(
    BinaryOp::Operator op, std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs
) const
{
    if (op == BinaryOp::Operator::Plus)
    {
        auto reference_type = dynamic_cast<ReferenceType*>(lhs.get());
        if (reference_type && *rhs == IntegerType{})
        {
            return lhs;
        }
    }

    if (!binary_operator_overloads_.contains(op))
    {
        throw SemanticError(std::format("No known overloads of binary operator '{}'.", str(op)));
    }

    auto& candidates = binary_operator_overloads_.at(op);
    auto matching_signature = std::find_if(
        candidates.cbegin(),
        candidates.cend(),
        [&](const BinaryOpSignature& signature) { return (*signature.lhs == *lhs) && (*signature.rhs == *rhs); }
    );

    if (matching_signature != candidates.cend())
    {
        return matching_signature->result;
    }

    throw SemanticError(std::format(
        "No viable overload of binary operator '{}' with left-hand side of type '{}' and right-hand side of type '{}'.",
        str(op),
        lhs->ToString(),
        rhs->ToString()
    ));
}

}  // namespace l0::detail
