#include "l0/semantics/typechecker.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

Typechecker::Typechecker(Module& module) : module_{module}
{
    simple_types_.insert(std::make_pair("Integer", std::make_unique<IntegerType>()));
    simple_types_.insert(std::make_pair("String", std::make_unique<StringType>()));
    simple_types_.insert(std::make_pair("Boolean", std::make_unique<BooleanType>()));
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
    auto declared = declaration.scope->GetType(declaration.variable);
    declaration.initializer->Accept(*this);
    auto init = declaration.initializer->type;

    if (*declared != *init)
    {
        throw SemanticError(std::format(
            "Variable '{}' is declared with type {}, but is initialized with value of type {}.",
            declaration.variable,
            declared->ToString(),
            init->ToString()
        ));
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
    if (*conditional_statement.condition->type != *simple_types_.at("Boolean"))
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
    if (*while_loop.condition->type != *simple_types_.at("Boolean"))
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

void Typechecker::Visit(const Assignment& assignment)
{
    auto declared = assignment.scope->GetType(assignment.variable);

    assignment.expression->Accept(*this);
    auto assigned = assignment.expression->type;

    if (*declared != *assigned)
    {
        throw SemanticError(std::format(
            "Variable '{}' was declared with type {}, but is assigned value of type {}.",
            assignment.variable,
            declared->ToString(),
            assigned->ToString()
        ));
    }

    assignment.type = declared;
}

void Typechecker::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    auto lhs = binary_op.left->type;

    binary_op.right->Accept(*this);
    auto rhs = binary_op.right->type;

    // TODO Uniformize and extract to separate class

    switch (binary_op.op)
    {
        case BinaryOp::Operator::Plus:
        {
            if (*lhs == *simple_types_.at("String") && *rhs == *simple_types_.at("String"))
            {
                binary_op.type = simple_types_.at("String");
                return;
            }
            else if (*lhs == *simple_types_.at("Integer") && *rhs == *simple_types_.at("Integer"))
            {
                binary_op.type = simple_types_.at("Integer");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '+' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
        case BinaryOp::Operator::Minus:
        {
            if (*lhs == *simple_types_.at("Integer") && *rhs == *simple_types_.at("Integer"))
            {
                binary_op.type = simple_types_.at("Integer");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '-' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
        case BinaryOp::Operator::Asterisk:
        {
            if (*lhs == *simple_types_.at("Integer") && *rhs == *simple_types_.at("Integer"))
            {
                binary_op.type = simple_types_.at("Integer");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '*' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
        case BinaryOp::Operator::AmpersandAmpersand:
        {
            if (*lhs == *simple_types_.at("Boolean") && *rhs == *simple_types_.at("Boolean"))
            {
                binary_op.type = simple_types_.at("Boolean");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '&&' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
        case BinaryOp::Operator::PipePipe:
        {
            if (*lhs == *simple_types_.at("Boolean") && *rhs == *simple_types_.at("Boolean"))
            {
                binary_op.type = simple_types_.at("Boolean");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '||' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
        case BinaryOp::Operator::EqualsEquals:
        {
            if (*lhs == *simple_types_.at("Integer") && *rhs == *simple_types_.at("Integer"))
            {
                binary_op.type = simple_types_.at("Boolean");
                return;
            }
            else if (*lhs == *simple_types_.at("Boolean") && *rhs == *simple_types_.at("Boolean"))
            {
                binary_op.type = simple_types_.at("Boolean");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '==' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
        case BinaryOp::Operator::BangEquals:
        {
            if (*lhs == *simple_types_.at("Integer") && *rhs == *simple_types_.at("Integer"))
            {
                binary_op.type = simple_types_.at("Boolean");
                return;
            }
            else if (*lhs == *simple_types_.at("Boolean") && *rhs == *simple_types_.at("Boolean"))
            {
                binary_op.type = simple_types_.at("Boolean");
                return;
            }
            else
            {
                throw SemanticError(std::format(
                    "No viable overload of operator '!=' for operands of type {} and {}.",
                    lhs->ToString(),
                    rhs->ToString()
                ));
            }
        }
    }
}

void Typechecker::Visit(const Variable& variable) { variable.type = variable.scope->GetType(variable.name); }

void Typechecker::Visit(const Call& call)
{
    call.function->Accept(*this);
    auto function = dynamic_cast<FunctionType*>(call.function->type.get());

    if (!function)
    {
        throw SemanticError(std::format("Cannot call value of non-function type {}.", call.function->type->ToString()));
    }

    if (call.arguments->size() != function->parameters->size())
    {
        throw SemanticError(std::format(
            "Expected {} arguments to function call, got {}.", function->parameters->size(), call.arguments->size()
        ));
    }

    for (auto i = 0zu; i < call.arguments->size(); ++i)
    {
        auto expected = function->parameters->at(i);
        call.arguments->at(i)->Accept(*this);
        auto actual = call.arguments->at(i)->type;

        if (*expected != *actual)
        {
            throw SemanticError(std::format(
                "Expected value of type {} as {}th argument, got {}.", expected->ToString(), i, actual->ToString()
            ));
        }
    }

    call.type = function->return_type;
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
    // TODO check return statements etc.
    for (const auto& statement : *function.statements)
    {
        statement->Accept(*this);
    }

    auto type = std::make_shared<FunctionType>();

    for (const auto& param_decl : *function.parameters)
    {
        type->parameters->push_back(param_decl->type);
    }
    type->return_type = function.return_type;

    function.type = type;
}

}  // namespace l0
