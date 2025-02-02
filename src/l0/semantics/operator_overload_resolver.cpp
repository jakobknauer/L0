#include "l0/semantics/operator_overload_resolver.h"

#include "l0/ast/ast_printer.h"
#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

OperatorOverloadResolver::OperatorOverloadResolver()
{
    auto boolean = std::make_shared<BooleanType>(TypeQualifier::Constant);
    auto integer = std::make_shared<IntegerType>(TypeQualifier::Constant);
    auto character = std::make_shared<CharacterType>(TypeQualifier::Constant);

    unary_operator_overloads_ = {
        {UnaryOp::Operator::Plus, {{integer, {integer, UnaryOp::Overload::IntegerIdentity}}}},
        {UnaryOp::Operator::Minus, {{integer, {integer, UnaryOp::Overload::IntegerNegation}}}},
        {UnaryOp::Operator::Bang, {{boolean, {boolean, UnaryOp::Overload::BooleanNegation}}}},
    };

    using BinOp = BinaryOp::Operator;
    using BinOverload = BinaryOp::Overload;
    binary_operator_overloads_ = {
        {
            BinOp::EqualsEquals,
            {
                {boolean, boolean, {boolean, BinOverload::BooleanEquality}},
                {integer, integer, {boolean, BinOverload::IntegerEquality}},
                {character, character, {boolean, BinOverload::CharacterEquality}},
            },
        },
        {
            BinOp::BangEquals,
            {
                {boolean, boolean, {boolean, BinOverload::BooleanInequality}},
                {integer, integer, {boolean, BinOverload::IntegerInequality}},
                {character, character, {boolean, BinOverload::CharacterInequality}},
            },
        },
        {
            BinOp::Plus,
            {
                {integer, integer, {integer, BinOverload::IntegerAddition}},
                {character, integer, {character, BinOverload::CharacterAddition}},
            },
        },
        {
            BinOp::Minus,
            {
                {integer, integer, {integer, BinOverload::IntegerSubtraction}},
                {character, character, {integer, BinOverload::CharacterSubtraction}},
            },
        },
        {BinOp::Asterisk, {{integer, integer, {integer, BinOverload::IntegerMultiplication}}}},
        {BinOp::Slash, {{integer, integer, {integer, BinOverload::IntegerDivision}}}},
        {BinOp::Percent, {{integer, integer, {integer, BinOverload::IntegerRemainder}}}},

        {BinOp::PipePipe, {{boolean, boolean, {boolean, BinOverload::BooleanDisjunction}}}},
        {BinOp::AmpersandAmpersand, {{boolean, boolean, {boolean, BinOverload::BooleanConjunction}}}},

        {BinOp::Less, {{integer, integer, {boolean, BinOverload::IntegerLess}}}},
        {BinOp::Greater, {{integer, integer, {boolean, BinOverload::IntegerGreater}}}},
        {BinOp::LessEquals, {{integer, integer, {boolean, BinOverload::IntegerLessOrEquals}}}},
        {BinOp::GreaterEquals, {{integer, integer, {boolean, BinOverload::IntegerGreaterOrEquals}}}},
    };
}

OperatorOverloadResolver::UnaryOpResolution OperatorOverloadResolver::ResolveUnaryOperator(
    UnaryOp::Operator op, std::shared_ptr<Type> operand
) const
{
    if (op == UnaryOp::Operator::Ampersand)
    {
        return {std::make_shared<ReferenceType>(operand, TypeQualifier::Constant), UnaryOp::Overload::AddressOf};
    }
    if (op == UnaryOp::Operator::Caret)
    {
        if (auto reference_type = dynamic_pointer_cast<ReferenceType>(operand))
        {
            return {reference_type->base_type, UnaryOp::Overload::Dereferenciation};
        }
        else
        {
            throw SemanticError(std::format("Cannot dereference value of type '{}'.", operand->ToString()));
        }
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
        return matching_signature->resolution;
    }

    throw SemanticError(std::format(
        "No viable overload of unary operator '{}' with operand of type '{}'.", str(op), operand->ToString()
    ));
}

OperatorOverloadResolver::BinaryOpResolution OperatorOverloadResolver::ResolveBinaryOperator(
    BinaryOp::Operator op, std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs
) const
{
    if (op == BinaryOp::Operator::Plus)
    {
        auto reference_type = dynamic_pointer_cast<ReferenceType>(lhs);
        if (reference_type && *rhs == IntegerType{TypeQualifier::Constant})
        {
            return {lhs, BinaryOp::Overload::ReferenceIndexation};
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
        return matching_signature->resolution;
    }

    throw SemanticError(std::format(
        "No viable overload of binary operator '{}' with left-hand side of type '{}' and right-hand side of type '{}'.",
        str(op),
        lhs->ToString(),
        rhs->ToString()
    ));
}

}  // namespace l0::detail
