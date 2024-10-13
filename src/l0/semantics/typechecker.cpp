#include "l0/semantics/typechecker.h"

#include <ranges>

#include "l0/common/constants.h"
#include "l0/semantics/semantic_error.h"

namespace l0
{

Typechecker::Typechecker(Module& module)
    : module_{module}
{
}

void Typechecker::Check()
{
    for (auto global_declaration : module_.global_declarations)
    {
        CheckGlobalDeclaration(*global_declaration);
    }

    for (auto type_name : module_.globals->GetTypes())
    {
        auto type = module_.globals->GetTypeDefinition(type_name);
        if (auto struct_type = dynamic_pointer_cast<StructType>(type))
        {
            CheckStruct(*struct_type);
        }
    }
}

void Typechecker::Visit(const StatementBlock& statement_block)
{
    for (const auto& statement : statement_block.statements)
    {
        statement->Accept(*this);
    }
}

void Typechecker::Visit(const Declaration& declaration)
{
    if (!declaration.initializer)
    {
        throw SemanticError(std::format("Local variable '{}' does not have an initializer.", declaration.variable));
    }

    declaration.initializer->Accept(*this);

    auto coerced_type = conversion_checker_.Coerce(declaration.annotation, declaration.initializer->type);

    if (!coerced_type)
    {
        throw SemanticError(std::format(
            "Could not coerce type annotation and initializer type for variable '{}'.", declaration.variable
        ));
    }

    declaration.scope->SetVariableType(declaration.variable, coerced_type);
}

void Typechecker::Visit(const TypeDeclaration& type_declaration)
{
    throw SemanticError("Unexpected type declaration.");
}

void Typechecker::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
}

void Typechecker::Visit(const ReturnStatement& return_statement)
{
    return_statement.value->Accept(*this);
}

void Typechecker::Visit(const ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);
    if (!conversion_checker_.CheckCompatibility(
            conditional_statement.condition->type, type_resolver_.Resolve(Typename::Boolean)
        ))
    {
        throw SemanticError(std::format(
            "Condition must be of type Boolean, but is of type '{}'.", conditional_statement.condition->type->ToString()
        ));
    }

    conditional_statement.then_block->Accept(*this);

    if (conditional_statement.else_block)
    {
        conditional_statement.else_block->Accept(*this);
    }
}

void Typechecker::Visit(const WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);
    if (!conversion_checker_.CheckCompatibility(while_loop.condition->type, type_resolver_.Resolve(Typename::Boolean)))
    {
        throw SemanticError(std::format(
            "Condition must be of type Boolean, but is of type '{}'.", while_loop.condition->type->ToString()
        ));
    }

    while_loop.body->Accept(*this);
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
        throw SemanticError(std::format("Cannot assign to target of constant type '{}'.", declared->ToString()));
    }

    assignment.expression->Accept(*this);
    auto assigned = assignment.expression->type;

    if (!conversion_checker_.CheckCompatibility(declared, assigned))
    {
        throw SemanticError(std::format(
            "Target of assignment is of type '{}', but is assigned value of incompatible type '{}'.",
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
    auto resolution = operator_overload_resolver_.ResolveUnaryOperator(unary_op.op, operand);
    unary_op.type = resolution.result_type;
    unary_op.overload = resolution.overload;
}

void Typechecker::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    auto lhs = binary_op.left->type;

    binary_op.right->Accept(*this);
    auto rhs = binary_op.right->type;

    auto resolution = operator_overload_resolver_.ResolveBinaryOperator(binary_op.op, lhs, rhs);
    binary_op.type = resolution.result_type;
    binary_op.overload = resolution.overload;
}

void Typechecker::Visit(const Variable& variable)
{
    variable.type = variable.scope->GetVariableType(variable.name);
}

void Typechecker::Visit(const MemberAccessor& member_accessor)
{
    member_accessor.object->Accept(*this);
    auto object_type = dynamic_pointer_cast<StructType>(member_accessor.object->type);
    if (!object_type)
    {
        throw SemanticError(std::format(
            "Member accessor object must be of struct type, but is of type '{}'.",
            member_accessor.object->type->ToString()
        ));
    }

    if (!object_type->HasMember(member_accessor.member))
    {
        throw SemanticError(
            std::format("Struct '{}' does not have a member named '{}'.", object_type->name, member_accessor.member)
        );
    }

    auto member = object_type->GetMember(member_accessor.member);
    auto member_type = member->type;
    if (object_type->mutability == TypeQualifier::Constant && member_type->mutability == TypeQualifier::Mutable)
    {
        member_type = ModifyQualifier(*member_type, TypeQualifier::Constant);
    }

    member_accessor.object_type = object_type;
    member_accessor.nonstatic_member_index = object_type->GetNonstaticMemberIndex(member_accessor.member);
    member_accessor.type = member_type;
}

void Typechecker::Visit(const Call& call)
{
    call.function->Accept(*this);

    if (IsMethodCall(call))
    {
        call.is_method_call = true;
        CheckMethodCall(call);
    }
    else
    {
        CheckFunctionCall(call);
    }
}

void Typechecker::Visit(const UnitLiteral& literal)
{
    literal.type = type_resolver_.Resolve(Typename::Unit);
}

void Typechecker::Visit(const BooleanLiteral& literal)
{
    literal.type = type_resolver_.Resolve(Typename::Boolean);
}

void Typechecker::Visit(const IntegerLiteral& literal)
{
    literal.type = type_resolver_.Resolve(Typename::Integer);
}

void Typechecker::Visit(const CharacterLiteral& literal)
{
    literal.type = type_resolver_.Resolve(Typename::Character);
}

void Typechecker::Visit(const StringLiteral& literal)
{
    literal.type = type_resolver_.Resolve(Typename::CString);
}

void Typechecker::Visit(const Function& function)
{
    if (function.captures)
    {
        for (const auto& [capture, scope] : std::views::zip(*function.captures, function.capture_scopes))
        {
            auto capture_type = scope->GetVariableType(capture);
            function.locals->SetVariableType(capture, capture_type);
        }
    }

    auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
    for (const auto& param_decl : *function.parameters)
    {
        auto param_type = type_resolver_.Convert(*param_decl->annotation);
        function.locals->SetVariableType(param_decl->name, param_type);
        parameters->push_back(param_type);
    }
    auto return_type = type_resolver_.Convert(*function.return_type_annotation);
    function.type = std::make_shared<FunctionType>(parameters, return_type, TypeQualifier::Constant);

    function.body->Accept(*this);
}

void Typechecker::Visit(const Initializer& initializer)
{
    auto annotated_type = type_resolver_.Convert(*initializer.annotation);
    auto struct_type = dynamic_pointer_cast<StructType>(annotated_type);
    if (!struct_type)
    {
        throw SemanticError(std::format(
            "Initializer type annotation must be of struct type, but is of type '{}'.", annotated_type->ToString()
        ));
    }

    std::unordered_set<std::string> explicitely_initialized_members{};
    for (const auto& member_initializer : *initializer.member_initializers)
    {
        const std::string& member_name = member_initializer->member;
        if (!struct_type->HasMember(member_name))
        {
            throw SemanticError(
                std::format("Struct '{}' does not have a member named '{}'.", struct_type->name, member_name)
            );
        }
        auto member = struct_type->GetMember(member_initializer->member);
        if (member->is_static)
        {
            throw SemanticError(
                std::format("Static member '{}' of struct '{}' cannot be initialized.", member->name, struct_type->name)
            );
        }

        if (explicitely_initialized_members.contains(member_name))
        {
            throw SemanticError(std::format("Member '{}' is initialized twice.", member_name));
        }
        explicitely_initialized_members.insert(member_name);

        member_initializer->value->Accept(*this);

        if (!conversion_checker_.CheckCompatibility(member->type, member_initializer->value->type))
        {
            throw SemanticError(std::format(
                "Target of assignment is of type '{}', but is assigned value of incompatible type '{}'.",
                member->type->ToString(),
                member_initializer->value->type->ToString()
            ));
        }
    }

    auto not_initialized_members =
        *struct_type->members
        | std::views::filter(
            [&](const auto& member)
            { return !member->default_initializer && !explicitely_initialized_members.contains(member->name); }
        )
        | std::views::transform([](const auto& member) { return member->name; })
        | std::ranges::to<std::unordered_set>();

    if (!not_initialized_members.empty())
    {
        throw SemanticError(std::format("There are {} uninitialized struct members.", not_initialized_members.size()));
    }

    initializer.type = annotated_type;
}

void Typechecker::Visit(const Allocation& allocation)
{
    if (allocation.size)
    {
        allocation.size->Accept(*this);
        auto integer_type = type_resolver_.Resolve(Typename::Integer);
        if (*allocation.size->type != *integer_type)
        {
            throw SemanticError(std::format(
                "Allocation size must be of type Integer, but is of type '{}'.", allocation.type->ToString()
            ));
        }
    }

    allocation.annotation->mutability = TypeAnnotationQualifier::Mutable;
    auto allocated_type = type_resolver_.Convert(*allocation.annotation);
    allocation.allocated_type = allocated_type;
    allocation.type = std::make_shared<ReferenceType>(allocation.allocated_type, TypeQualifier::Constant);

    if (allocation.member_initializers)
    {
        auto initializer = std::make_shared<Initializer>(allocation.annotation, allocation.member_initializers);
        allocation.initial_value = initializer;
    }
    else
    {
        allocation.initial_value = GetInitialValue(allocated_type);
        allocation.initial_value->Accept(*this);
    }
    allocation.initial_value->Accept(*this);
}

std::shared_ptr<Expression> Typechecker::GetInitialValue(std::shared_ptr<Type> type) const
{
    if (dynamic_pointer_cast<UnitType>(type))
    {
        return std::make_shared<UnitLiteral>();
    }
    if (dynamic_pointer_cast<BooleanType>(type))
    {
        return std::make_shared<BooleanLiteral>(false);
    }
    if (dynamic_pointer_cast<IntegerType>(type))
    {
        return std::make_shared<IntegerLiteral>(0);
    }
    if (dynamic_pointer_cast<CharacterType>(type))
    {
        return std::make_shared<CharacterLiteral>('\0');
    }

    throw SemanticError(std::format("Cannot create initial value of type '{}'.", type->ToString()));
}

bool Typechecker::IsMethodCall(const Call& call) const
{
    auto member_accessor = dynamic_pointer_cast<MemberAccessor>(call.function);

    if (!member_accessor)
    {
        return false;
    }

    auto member = member_accessor->object_type->GetMember(member_accessor->member);
    return member->is_method;
}

void Typechecker::CheckFunctionCall(const Call& call)
{
    auto function_type = dynamic_pointer_cast<FunctionType>(call.function->type);
    if (!function_type)
    {
        throw SemanticError(std::format("Cannot call value of non-function type {}.", call.function->type->ToString()));
    }
    auto parameter_types = std::views::all(*function_type->parameters);

    std::ranges::for_each(*call.arguments, [&](auto argument) { argument->Accept(*this); });
    auto argument_types = *call.arguments | std::views::transform([](auto argument) { return argument->type; });

    if (parameter_types.size() != argument_types.size())
    {
        throw SemanticError(std::format(
            "Expected {} arguments to function call, got {}.", parameter_types.size(), argument_types.size()
        ));
    }

    auto indices = std::views::iota(1);

    for (auto [index, expected, actual] : std::views::zip(indices, parameter_types, argument_types))
    {
        if (!conversion_checker_.CheckCompatibility(expected, actual))
        {
            throw SemanticError(std::format(
                "Expected value of type '{}' as argument #{}, got incompatible type '{}' instead.",
                expected->ToString(),
                index,
                actual->ToString()
            ));
        }
    }

    call.type = function_type->return_type;
}

void Typechecker::CheckMethodCall(const Call& call)
{
    auto function_type = dynamic_pointer_cast<FunctionType>(call.function->type);
    if (!function_type)
    {
        throw SemanticError(std::format("Cannot call value of non-function type {}.", call.function->type->ToString()));
    }
    auto parameter_types = std::views::all(*function_type->parameters);

    std::vector<std::shared_ptr<Type>> argument_types{};

    auto this_type = dynamic_pointer_cast<MemberAccessor>(call.function)->object_type;
    argument_types.push_back(std::make_shared<ReferenceType>(this_type, TypeQualifier::Mutable));

    std::ranges::for_each(*call.arguments, [&](auto argument) { argument->Accept(*this); });
    auto explicit_arguments = *call.arguments | std::views::transform([](auto argument) { return argument->type; })
                            | std::ranges::to<std::vector>();
    argument_types.insert(argument_types.end(), explicit_arguments.begin(), explicit_arguments.end());

    if (parameter_types.size() != argument_types.size())
    {
        throw SemanticError(
            std::format("Expected {} arguments to method call, got {}.", parameter_types.size(), argument_types.size())
        );
    }

    auto indices = std::views::iota(1);

    for (auto [index, expected, actual] : std::views::zip(indices, parameter_types, argument_types))
    {
        if (!conversion_checker_.CheckCompatibility(expected, actual))
        {
            throw SemanticError(std::format(
                "Expected value of type '{}' as argument #{}, got incompatible type '{}' instead.",
                expected->ToString(),
                index,
                actual->ToString()
            ));
        }
    }

    call.type = function_type->return_type;
}

void Typechecker::CheckGlobalDeclaration(const Declaration& declaration)
{
    declaration.initializer->Accept(*this);
    auto initializer_type = declaration.initializer->type;

    auto declared_type = module_.globals->GetVariableType(declaration.variable);

    if (!conversion_checker_.CheckCompatibility(declared_type, initializer_type))
    {
        throw SemanticError(std::format(
            "Global variable '{}' is declared with type '{}', but initializer is of incompatible type '{}'.",
            declaration.variable,
            declared_type->ToString(),
            initializer_type->ToString()
        ));
    }
}

void Typechecker::CheckStruct(const StructType& struct_type)
{
    for (const auto& member : *struct_type.members)
    {
        if (!member->default_initializer)
        {
            continue;
        }

        member->default_initializer->Accept(*this);
        auto initializer_type = member->default_initializer->type;
        auto annotated_type = member->type;

        if (!conversion_checker_.CheckCompatibility(annotated_type, initializer_type))
        {
            throw SemanticError(std::format(
                "Member '{}' is declared with type '{}', but default initializer has incompatible type '{}'.",
                member->name,
                annotated_type->ToString(),
                initializer_type->ToString()
            ));
        }
    }
}

}  // namespace l0
