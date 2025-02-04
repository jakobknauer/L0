#include "l0/semantics/reference_pass.h"

#include "l0/semantics/semantic_error.h"

namespace l0
{

ReferencePass::ReferencePass(Module& module)
    : module_{module}
{
}

void ReferencePass::Run()
{
    for (auto callable : module_.callables)
    {
        callable->Accept(*this);
    }
}

void ReferencePass::Visit(StatementBlock& statement_block)
{
    for (auto statement : statement_block.statements)
    {
        statement->Accept(*this);
    }
}

void ReferencePass::Visit(Declaration& declaration)
{
    declaration.initializer->Accept(*this);
}

void ReferencePass::Visit(TypeDeclaration& type_declaration)
{
    type_declaration.definition->Accept(*this);
}

void ReferencePass::Visit(ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
}

void ReferencePass::Visit(ReturnStatement& return_statement)
{
    return_statement.value->Accept(*this);
}

void ReferencePass::Visit(ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);
    conditional_statement.then_block->Accept(*this);

    if (conditional_statement.else_block)
    {
        conditional_statement.else_block->Accept(*this);
    }
}

void ReferencePass::Visit(WhileLoop& while_loop)
{
    while_loop.condition->Accept(*this);
    while_loop.body->Accept(*this);
}

void ReferencePass::Visit(Deallocation& deallocation)
{
    deallocation.reference->Accept(*this);
}

void ReferencePass::Visit(Assignment& assignment)
{
    assignment.expression->Accept(*this);
    assignment.target->Accept(*this);

    if (!IsLValue(assignment.target))
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

    if (!IsLValue(unary_op.operand))
    {
        throw SemanticError("Can only create references to lvalues.");
    }
}

void ReferencePass::Visit(BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    binary_op.right->Accept(*this);
}

void ReferencePass::Visit(Variable&) {}

void ReferencePass::Visit(MemberAccessor& member_accessor)
{
    member_accessor.object->Accept(*this);
}

void ReferencePass::Visit(Call& call)
{
    call.function->Accept(*this);
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
    }
}

void ReferencePass::Visit(UnitLiteral&) {}
void ReferencePass::Visit(BooleanLiteral&) {}
void ReferencePass::Visit(IntegerLiteral&) {}
void ReferencePass::Visit(CharacterLiteral&) {}
void ReferencePass::Visit(StringLiteral&) {}

void ReferencePass::Visit(Function& function)
{
    function.body->Accept(*this);
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
    if (allocation.member_initializers)
    {
        for (const auto& member_initializer : *allocation.member_initializers)
        {
            member_initializer->value->Accept(*this);
        }
    }
}

void ReferencePass::Visit(StructExpression& struct_expression)
{
    for (const auto& member_declaration : *struct_expression.members)
    {
        if (member_declaration->initializer)
        {
            member_declaration->initializer->Accept(*this);
        }
    }
}

void ReferencePass::Visit(EnumExpression&) {}

bool ReferencePass::IsLValue(std::shared_ptr<Expression> value) const
{
    if (auto variable = dynamic_pointer_cast<Variable>(value))
    {
        return true;
    }
    else if (auto unary_op = dynamic_pointer_cast<UnaryOp>(value);
             unary_op && (unary_op->op == UnaryOp::Operator::Caret))
    {
        return true;
    }
    else if (auto member_accessor = dynamic_pointer_cast<MemberAccessor>(value))
    {
        return member_accessor->nonstatic_member_index.has_value() && IsLValue(member_accessor->object);
    }
    return false;
}

}  // namespace l0
