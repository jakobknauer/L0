#include "l0/semantics/reference_pass.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

ReferencePass::ReferencePass(Module& module) : module_{module} {}

void ReferencePass::Run()
{
    for (auto& statement : *module_.statements)
    {
        statement->Accept(*this);
    }
}

void ReferencePass::Visit(Declaration& declaration) { declaration.initializer->Accept(*this); }

void ReferencePass::Visit(TypeDeclaration& type_declaration) { type_declaration.definition->Accept(*this); }

void ReferencePass::Visit(ExpressionStatement& expression_statement) { expression_statement.expression->Accept(*this); }

void ReferencePass::Visit(ReturnStatement& return_statement) { return_statement.value->Accept(*this); }

void ReferencePass::Visit(ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);
    for (auto& statement : *conditional_statement.then_block)
    {
        statement->Accept(*this);
    }

    if (!conditional_statement.else_block)
    {
        return;
    }
    for (auto& statement : *conditional_statement.else_block)
    {
        statement->Accept(*this);
    }
}

void ReferencePass::Visit(WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);
    for (auto& statement : *while_loop.body)
    {
        statement->Accept(*this);
    }
}

void ReferencePass::Visit(Deallocation& deallocation) { deallocation.reference->Accept(*this); }

void ReferencePass::Visit(Assignment& assignment)
{
    assignment.expression->Accept(*this);
    assignment.target->Accept(*this);

    if (!IsLValue(assignment.target, assignment.target_address))
    {
        throw SemanticError("Can only assign to lvalues.");
    }
}

void ReferencePass::Visit(UnaryOp& unary_op)
{
    unary_op.operand->Accept(*this);

    std::shared_ptr<Expression> _;
    if (unary_op.op != UnaryOp::Operator::Ampersand)
    {
        return;
    }

    AddressInfo unused;
    if (!IsLValue(unary_op.operand, unused))
    {
        throw SemanticError("Can only create references to lvalues.");
    }
}

void ReferencePass::Visit(BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    binary_op.right->Accept(*this);
}

void ReferencePass::Visit(Variable& variable) {}

void ReferencePass::Visit(MemberAccessor& member_accessor) { member_accessor.object->Accept(*this); }

void ReferencePass::Visit(Call& call)
{
    call.function->Accept(*this);
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
    }
}

void ReferencePass::Visit(UnitLiteral& literal) {}
void ReferencePass::Visit(BooleanLiteral& literal) {}
void ReferencePass::Visit(IntegerLiteral& literal) {}
void ReferencePass::Visit(StringLiteral& literal) {}

void ReferencePass::Visit(Function& function)
{
    for (auto& statement : *function.statements)
    {
        statement->Accept(*this);
    }
}

void ReferencePass::Visit(Initializer& initializer)
{
    for (const auto& member_initializer : *initializer.member_initializers)
    {
        member_initializer->value->Accept(*this);
    }
}

void ReferencePass::Visit(Allocation& allocation)
{
    if (allocation.size)
    {
        allocation.size->Accept(*this);
    }
}

void ReferencePass::Visit(StructExpression& struct_expression)
{
    for (const auto& statement : *struct_expression.body)
    {
        auto member_declaration = dynamic_pointer_cast<Declaration>(statement);
        if (!member_declaration)
        {
            throw SemanticError("Expected declaration as statement in struct expression body.");
        }
        member_declaration->initializer->Accept(*this);
    }
}

bool ReferencePass::IsLValue(std::shared_ptr<Expression> value, AddressInfo& out_address) const
{
    if (auto variable = dynamic_pointer_cast<Variable>(value))
    {
        out_address.object_ref = std::make_shared<UnaryOp>(variable, UnaryOp::Operator::Ampersand);
        out_address.object_ref->type = std::make_shared<ReferenceType>(variable->type, TypeQualifier::Constant);
        out_address.object_type = variable->type;
        return true;
    }
    else if (auto unary_op = dynamic_pointer_cast<UnaryOp>(value);
             unary_op && (unary_op->op == UnaryOp::Operator::Asterisk))
    {
        out_address.object_ref = unary_op->operand;
        out_address.object_type = unary_op->type;
        return true;
    }
    else if (auto member_accessor = dynamic_pointer_cast<MemberAccessor>(value))
    {
        if (IsLValue(member_accessor->object, out_address))
        {
            out_address.member_indices.push_back(member_accessor->member_index);
            return true;
        }
        return false;
    }
    return false;
}

}  // namespace l0
