#include "l0/semantics/typechecker.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

Typechecker::Typechecker(Module& module) : module_{module}
{
    simple_types_.insert(std::make_pair("Unit", std::make_shared<UnitType>(TypeQualifier::Constant)));
    simple_types_.insert(std::make_pair("Integer", std::make_shared<IntegerType>(TypeQualifier::Constant)));
    simple_types_.insert(std::make_pair("String", std::make_shared<StringType>(TypeQualifier::Constant)));
    simple_types_.insert(std::make_pair("Boolean", std::make_shared<BooleanType>(TypeQualifier::Constant)));
}

void Typechecker::Check()
{
    for (const auto& statement : *module_.statements)
    {
        statement->Accept(*this);
    }
}

void Typechecker::Visit(const Declaration& declaration)
{
    declaration.initializer->Accept(*this);

    if (declaration.scope->IsTypeSet(declaration.variable))
    {
        return;
    }

    auto initializer_type = declaration.initializer->type;
    if (declaration.annotation)
    {
        auto annotated_type = converter_.Convert(*declaration.annotation);
        if (!conversion_checker_.CheckCompatibility(annotated_type, initializer_type))
        {
            throw SemanticError(std::format(
                "Variable '{}' is declared with type {}, but is initialized with value of incompatible type {}.",
                declaration.variable,
                annotated_type->ToString(),
                initializer_type->ToString()
            ));
        }
        declaration.scope->SetType(declaration.variable, annotated_type);
    }
    else
    {
        auto const_initializer_type = ModifyQualifier(*initializer_type, TypeQualifier::Constant);
        declaration.scope->SetType(declaration.variable, const_initializer_type);
    }
}

void Typechecker::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
}

void Typechecker::Visit(const ReturnStatement& return_statement) { return_statement.value->Accept(*this); }

void Typechecker::Visit(const ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);
    if (!conversion_checker_.CheckCompatibility(conditional_statement.condition->type, simple_types_.at("Boolean")))
    {
        throw SemanticError(std::format(
            "Condition must be of type Boolean, but is of type '{}'.", conditional_statement.condition->type->ToString()
        ));
    }

    for (const auto& statement : *conditional_statement.then_block)
    {
        statement->Accept(*this);
    }

    if (!conditional_statement.else_block)
    {
        return;
    }
    for (const auto& statement : *conditional_statement.else_block)
    {
        statement->Accept(*this);
    }
}

void Typechecker::Visit(const WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);
    if (!conversion_checker_.CheckCompatibility(while_loop.condition->type, simple_types_.at("Boolean")))
    {
        throw SemanticError(std::format(
            "Condition must be of type Boolean, but is of type '{}'.", while_loop.condition->type->ToString()
        ));
    }

    for (const auto& statement : *while_loop.body)
    {
        statement->Accept(*this);
    }
}

void Typechecker::Visit(const Deallocation& deallocation)
{
    deallocation.reference->Accept(*this);
    auto reference_type = dynamic_pointer_cast<ReferenceType>(deallocation.reference->type);
    if (!reference_type)
    {
        throw SemanticError(std::format(
            "Operand of delete statement must be of reference type, but is of type '{}'.",
            deallocation.reference->type->ToString()
        ));
    }
}

void Typechecker::Visit(const Assignment& assignment)
{
    assignment.target->Accept(*this);
    auto declared = assignment.target->type;

    if (declared->mutability == TypeQualifier::Constant)
    {
        throw SemanticError(std::format("Cannot assign to target of constant type {}.", declared->ToString()));
    }

    assignment.expression->Accept(*this);
    auto assigned = assignment.expression->type;

    if (!conversion_checker_.CheckCompatibility(declared, assigned))
    {
        throw SemanticError(std::format(
            "Target of assignment is of type {}, but is assigned value of incompatible type {}.",
            declared->ToString(),
            assigned->ToString()
        ));
    }

    assignment.type = declared;
}

void Typechecker::Visit(const UnaryOp& unary_op)
{
    unary_op.operand->Accept(*this);
    auto operand = unary_op.operand->type;
    unary_op.type = operator_overload_resolver_.ResolveUnaryOperator(unary_op.op, operand);
}

void Typechecker::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    auto lhs = binary_op.left->type;

    binary_op.right->Accept(*this);
    auto rhs = binary_op.right->type;

    binary_op.type = operator_overload_resolver_.ResolveBinaryOperator(binary_op.op, lhs, rhs);
}

void Typechecker::Visit(const Variable& variable) { variable.type = variable.scope->GetType(variable.name); }

void Typechecker::Visit(const Call& call)
{
    call.function->Accept(*this);
    auto function_type = dynamic_pointer_cast<FunctionType>(call.function->type);

    if (!function_type)
    {
        throw SemanticError(std::format("Cannot call value of non-function type {}.", call.function->type->ToString()));
    }

    if (call.arguments->size() != function_type->parameters->size())
    {
        throw SemanticError(std::format(
            "Expected {} arguments to function call, got {}.", function_type->parameters->size(), call.arguments->size()
        ));
    }

    for (auto i = 0zu; i < call.arguments->size(); ++i)
    {
        auto expected = function_type->parameters->at(i);
        call.arguments->at(i)->Accept(*this);
        auto actual = call.arguments->at(i)->type;

        if (!conversion_checker_.CheckCompatibility(expected, actual))
        {
            throw SemanticError(std::format(
                "Expected value of type {} as argument #{}, got incompatible type {} instead.",
                expected->ToString(),
                i,
                actual->ToString()
            ));
        }
    }

    call.type = function_type->return_type;
}

void Typechecker::Visit(const UnitLiteral& literal)
{
    auto unit_type = simple_types_.at("Unit");
    literal.type = unit_type;
}

void Typechecker::Visit(const BooleanLiteral& literal)
{
    auto boolean_type = simple_types_.at("Boolean");
    literal.type = boolean_type;
}

void Typechecker::Visit(const IntegerLiteral& literal)
{
    auto integer_type = simple_types_.at("Integer");
    literal.type = integer_type;
}

void Typechecker::Visit(const StringLiteral& literal)
{
    auto string_type = simple_types_.at("String");
    literal.type = string_type;
}

void Typechecker::Visit(const Function& function)
{
    auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
    for (const auto& param_decl : *function.parameters)
    {
        auto param_type = converter_.Convert(*param_decl->annotation);
        function.locals->SetType(param_decl->name, param_type);
        parameters->push_back(param_type);
    }
    auto return_type = converter_.Convert(*function.return_type_annotation);
    function.type = std::make_shared<FunctionType>(parameters, return_type, TypeQualifier::Constant);

    for (const auto& statement : *function.statements)
    {
        statement->Accept(*this);
    }
}

void Typechecker::Visit(const Allocation& allocation)
{
    if (allocation.size)
    {
        allocation.size->Accept(*this);
        auto integer_type = simple_types_.at("Integer");
        if (*allocation.size->type != *integer_type)
        {
            throw SemanticError(
                std::format("Allocation size must be of type Integer, but is of type {}.", allocation.type->ToString())
            );
        }
    }

    allocation.annotation->mutability = TypeAnnotationQualifier::Mutable;
    allocation.allocated_type = converter_.Convert(*allocation.annotation);
    allocation.type = std::make_shared<ReferenceType>(allocation.allocated_type, TypeQualifier::Constant);
}

}  // namespace l0
