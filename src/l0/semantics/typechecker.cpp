#include "l0/semantics/typechecker.h"

#include <ranges>

#include "l0/common/constants.h"
#include "l0/semantics/semantic_error.h"

namespace l0::detail
{

Typechecker::Typechecker(Module& module)
    : module_{module}
{
}

void Typechecker::Run()
{
    for (auto global_declaration : module_.global_declarations)
    {
        namespaces_.push(global_declaration->identifier.GetPrefix());
        CheckGlobalDeclaration(*global_declaration);
        namespaces_.pop();
    }

    for (auto global_type_declaration : module_.global_type_declarations)
    {
        auto type = global_type_declaration->type;
        if (auto struct_type = dynamic_pointer_cast<StructType>(type))
        {
            namespaces_.push(global_type_declaration->identifier.GetPrefix());
            CheckStruct(*struct_type);
            namespaces_.pop();
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
        throw SemanticError(
            std::format("Local variable '{}' does not have an initializer.", declaration.identifier.ToString())
        );
    }

    declaration.initializer->Accept(*this);

    auto coerced_type =
        conversion_checker_.Coerce(declaration.annotation, declaration.initializer->type, namespaces_.top());

    if (!coerced_type)
    {
        throw SemanticError(std::format(
            "Could not coerce type annotation and initializer type for variable '{}'.",
            declaration.identifier.ToString()
        ));
    }

    declaration.scope->SetVariableType(declaration.identifier, coerced_type);
}

void Typechecker::Visit(const TypeDeclaration&)
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
            conditional_statement.condition->type, type_resolver_.GetTypeByName(Typename::Boolean, Identifier{})
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
    if (!conversion_checker_.CheckCompatibility(
            while_loop.condition->type, type_resolver_.GetTypeByName(Typename::Boolean, Identifier{})
        ))
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

    if (dynamic_pointer_cast<ReferenceType>(deallocation.reference->type))
    {
        deallocation.deallocation_type = Deallocation::DeallocationType::Reference;
    }
    else if (dynamic_pointer_cast<FunctionType>(deallocation.reference->type))
    {
        deallocation.deallocation_type = Deallocation::DeallocationType::Closure;
    }
    else
    {
        throw SemanticError(std::format(
            "Operand of delete statement must be of reference or function type, but is of type '{}'.",
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
    variable.type = variable.scope->GetVariableType(variable.resolved_name);
}

void Typechecker::Visit(const MemberAccessor& member_accessor)
{
    member_accessor.object->Accept(*this);
    std::shared_ptr<Type> dereferenced_object_type = member_accessor.object->type;
    std::shared_ptr<Expression> dereferenced_object = member_accessor.object;
    while (auto type_as_ref = dynamic_pointer_cast<ReferenceType>(dereferenced_object_type))
    {
        dereferenced_object_type = type_as_ref->base_type;

        auto new_dereferenced_object = std::make_shared<UnaryOp>(dereferenced_object, UnaryOp::Operator::Caret);
        new_dereferenced_object->overload = UnaryOp::Overload::Dereferenciation;
        new_dereferenced_object->type = dereferenced_object_type;
        dereferenced_object = new_dereferenced_object;
    }

    auto dereferenced_object_type_as_struct = dynamic_pointer_cast<StructType>(dereferenced_object_type);
    if (!dereferenced_object_type_as_struct)
    {
        throw SemanticError(std::format(
            "Type of member accessor object after removing references must be of struct type, but is of type '{}'.",
            dereferenced_object_type->ToString()
        ));
    }

    if (!dereferenced_object_type_as_struct->HasMember(member_accessor.member))
    {
        throw SemanticError(std::format(
            "Struct '{}' does not have a member named '{}'.",
            dereferenced_object_type_as_struct->identifier.ToString(),
            member_accessor.member
        ));
    }

    auto member = dereferenced_object_type_as_struct->GetMember(member_accessor.member);
    auto member_type = member->type;
    if (dereferenced_object_type_as_struct->mutability == TypeQualifier::Constant
        && member_type->mutability == TypeQualifier::Mutable)
    {
        member_type = ModifyQualifier(*member_type, TypeQualifier::Constant);
    }

    member_accessor.dereferenced_object_type = dereferenced_object_type_as_struct;
    member_accessor.dereferenced_object = dereferenced_object;
    member_accessor.dereferenced_object_type_scope =
        type_resolver_.Resolve(member_accessor.dereferenced_object_type->identifier, Identifier{})
            .first;  // TODO find clean solution
    member_accessor.nonstatic_member_index =
        dereferenced_object_type_as_struct->GetNonstaticMemberIndex(member_accessor.member);
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
    literal.type = type_resolver_.GetTypeByName(Typename::Unit, Identifier{});
}

void Typechecker::Visit(const BooleanLiteral& literal)
{
    literal.type = type_resolver_.GetTypeByName(Typename::Boolean, Identifier{});
}

void Typechecker::Visit(const IntegerLiteral& literal)
{
    literal.type = type_resolver_.GetTypeByName(Typename::Integer, Identifier{});
}

void Typechecker::Visit(const CharacterLiteral& literal)
{
    literal.type = type_resolver_.GetTypeByName(Typename::Character, Identifier{});
}

void Typechecker::Visit(const StringLiteral& literal)
{
    literal.type = type_resolver_.GetTypeByName(Typename::CString, Identifier{});
}

void Typechecker::Visit(const Function& function)
{
    if (function.captures)
    {
        for (const auto& capture : *function.captures)
        {
            capture->Accept(*this);
            auto capture_type = capture->type;
            function.locals->SetVariableType(capture->name, capture_type);
        }
    }

    auto parameters = std::make_shared<std::vector<std::shared_ptr<Type>>>();
    for (const auto& param_decl : *function.parameters)
    {
        auto param_type = type_resolver_.Convert(*param_decl->annotation, namespaces_.top());
        function.locals->SetVariableType(param_decl->name, param_type);
        parameters->push_back(param_type);
    }
    auto return_type = type_resolver_.Convert(*function.return_type_annotation, namespaces_.top());
    function.type = std::make_shared<FunctionType>(parameters, return_type, TypeQualifier::Constant);

    function.body->Accept(*this);
}

void Typechecker::Visit(const Initializer& initializer)
{
    auto annotated_type = type_resolver_.Convert(*initializer.annotation, namespaces_.top());
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
            throw SemanticError(std::format(
                "Struct '{}' does not have a member named '{}'.", struct_type->identifier.ToString(), member_name
            ));
        }
        auto member = struct_type->GetMember(member_initializer->member);
        if (member->is_static)
        {
            throw SemanticError(std::format(
                "Static member '{}' of struct '{}' cannot be initialized.",
                member->name,
                struct_type->identifier.ToString()
            ));
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
    initializer.type_scope = type_resolver_.Resolve(struct_type->identifier, namespaces_.top()).first;
}

void Typechecker::Visit(const Allocation& allocation)
{
    if (allocation.size)
    {
        allocation.size->Accept(*this);
        auto integer_type = type_resolver_.GetTypeByName(Typename::Integer, Identifier{});
        if (*allocation.size->type != *integer_type)
        {
            throw SemanticError(std::format(
                "Allocation size must be of type Integer, but is of type '{}'.", allocation.type->ToString()
            ));
        }
    }

    allocation.annotation->mutability = TypeAnnotationQualifier::Mutable;
    auto allocated_type = type_resolver_.Convert(*allocation.annotation, namespaces_.top());
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

    auto member = member_accessor->dereferenced_object_type->GetMember(member_accessor->member);
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

    auto this_type = dynamic_pointer_cast<MemberAccessor>(call.function)->dereferenced_object_type;
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

    auto declared_type = module_.globals->GetVariableType(declaration.identifier);

    if (!conversion_checker_.CheckCompatibility(declared_type, initializer_type))
    {
        throw SemanticError(std::format(
            "Global variable '{}' is declared with type '{}', but initializer is of incompatible type '{}'.",
            declaration.identifier.ToString(),
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
        auto annotated_type = member->type;
        auto initializer_type = member->default_initializer->type;

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

}  // namespace l0::detail
