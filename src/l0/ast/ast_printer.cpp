#include "l0/ast/ast_printer.h"

#include <ranges>

namespace l0
{

template <std::ranges::input_range R, class UnaryFunc, class InterleaveUnaryFunc>
void interleaved_for_each(R&& r, UnaryFunc f, InterleaveUnaryFunc interleave)
{
    auto it = r.begin();
    if (it == r.end())
    {
        return;
    }

    f(*it);
    ++it;

    for (; it != r.end(); ++it)
    {
        interleave(*it);
        f(*it);
    }
}

AstPrinter::AstPrinter(std::ostream& out)
    : out_{out},
      indent_{out_}
{
}

void AstPrinter::Print(Module& module)
{
    for (const auto& statement : *module.statements)
    {
        Print(*statement);
    }
};

void AstPrinter::Print(Statement& statement)
{
    statement.Accept(*this);
    out_ << ";\n";
}

void AstPrinter::Print(Expression& expression)
{
    expression.Accept(*this);
}

void AstPrinter::Visit(const Declaration& declaration)
{
    out_ << declaration.variable;
    out_ << " :";

    if (declaration.annotation)
    {
        out_ << " ";
        declaration.annotation->Accept(*this);
    }

    if (declaration.annotation && declaration.initializer)
    {
        out_ << " ";
    }

    if (declaration.initializer)
    {
        out_ << "= ";
        declaration.initializer->Accept(*this);
    }
}

void AstPrinter::Visit(const TypeDeclaration& type_declaration)
{
    out_ << type_declaration.name << " : type = ";
    type_declaration.definition->Accept(*this);
}

void AstPrinter::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
}

void AstPrinter::Visit(const ReturnStatement& return_statement)
{
    out_ << "return ";
    return_statement.value->Accept(*this);
}

void AstPrinter::Visit(const ConditionalStatement& conditional_statement)
{
    out_ << "if ";
    conditional_statement.condition->Accept(*this);
    out_ << ":\n";

    out_ << "{\n";
    ++indent_;
    for (const auto& statement : *conditional_statement.then_block)
    {
        Print(*statement);
    }
    --indent_;
    out_ << "}";

    if (!conditional_statement.else_block)
    {
        return;
    }

    out_ << "\nelse\n";
    out_ << "{\n";
    ++indent_;
    for (const auto& statement : *conditional_statement.else_block)
    {
        Print(*statement);
    }
    --indent_;
    out_ << "}";
}

void AstPrinter::Visit(const WhileLoop& while_loop)
{
    out_ << "while ";
    while_loop.condition->Accept(*this);
    out_ << ":\n";

    out_ << "{\n";
    ++indent_;
    for (const auto& statement : *while_loop.body)
    {
        Print(*statement);
    }
    --indent_;
    out_ << "}";
}

void AstPrinter::Visit(const Deallocation& deallocation)
{
    out_ << "delete ";
    deallocation.reference->Accept(*this);
}

void AstPrinter::Visit(const Assignment& assignment)
{
    assignment.target->Accept(*this);
    out_ << " = ";
    assignment.expression->Accept(*this);
}

void AstPrinter::Visit(const UnaryOp& unary_op)
{
    out_ << "(" << str(unary_op.op);
    unary_op.operand->Accept(*this);
    out_ << ")";
}

void AstPrinter::Visit(const BinaryOp& binary_op)
{
    out_ << "(";
    binary_op.left->Accept(*this);
    out_ << " " << str(binary_op.op) << " ";
    binary_op.right->Accept(*this);
    out_ << ")";
}

void AstPrinter::Visit(const Variable& variable)
{
    out_ << variable.name;
}

void AstPrinter::Visit(const MemberAccessor& member_accessor)
{
    member_accessor.object->Accept(*this);
    out_ << "." << member_accessor.member;
}

void AstPrinter::Visit(const Call& call)
{
    call.function->Accept(*this);
    out_ << "(";
    interleaved_for_each(
        *call.arguments,
        [&](const auto& argument) { argument->Accept(*this); },
        [&](const auto& argument) { out_ << ", "; }
    );
    out_ << ")";
}

void AstPrinter::Visit(const UnitLiteral& literal)
{
    out_ << "unit";
}

void AstPrinter::Visit(const BooleanLiteral& literal)
{
    out_ << (literal.value ? "true" : "false");
}

void AstPrinter::Visit(const IntegerLiteral& literal)
{
    out_ << literal.value;
}

void AstPrinter::Visit(const StringLiteral& literal)
{
    out_ << "\"" << literal.value << "\"";
}

void AstPrinter::Visit(const Function& function)
{
    out_ << "$(";
    interleaved_for_each(
        *function.parameters,
        [&](const auto& parameter)
        {
            out_ << parameter->name << ": ";
            parameter->annotation->Accept(*this);
        },
        [&](const auto& parameter) { out_ << ", "; }
    );
    out_ << ") -> ";
    function.return_type_annotation->Accept(*this);
    out_ << "\n{\n";
    ++indent_;
    for (auto& statement : *function.statements)
    {
        statement->Accept(*this);
        out_ << ";\n";
    }
    --indent_;
    out_ << "}";
}

void AstPrinter::Visit(const Initializer& initializer)
{
    initializer.annotation->Accept(*this);
    if (initializer.member_initializers->empty())
    {
        out_ << "{}";
        return;
    }

    out_ << "\n{\n";
    ++indent_;
    for (const auto& member_initializer : *initializer.member_initializers)
    {
        out_ << member_initializer->member << " = ";
        member_initializer->value->Accept(*this);
        out_ << ";\n";
    }
    --indent_;
    out_ << "}";
}

void AstPrinter::Visit(const Allocation& allocation)
{
    out_ << "new";
    if (allocation.size)
    {
        out_ << "[";
        allocation.size->Accept(*this);
        out_ << "]";
    }
    out_ << " ";
    allocation.annotation->Accept(*this);

    if (!allocation.member_initializers)
    {
        return;
    }

    if (allocation.member_initializers->empty())
    {
        out_ << "{}";
        return;
    }

    out_ << "\n{\n";
    ++indent_;
    for (const auto& member_initializer : *allocation.member_initializers)
    {
        out_ << member_initializer->member << " = ";
        member_initializer->value->Accept(*this);
        out_ << ";\n";
    }
    --indent_;
    out_ << "}";
}

void AstPrinter::Visit(const SimpleTypeAnnotation& sta)
{
    PrintQualifier(sta.mutability);
    out_ << sta.type;
}

void AstPrinter::Visit(const ReferenceTypeAnnotation& rta)
{
    PrintQualifier(rta.mutability);
    out_ << "&";
    rta.base_type->Accept(*this);
}

void AstPrinter::Visit(const FunctionTypeAnnotation& fta)
{
    PrintQualifier(fta.mutability);
    out_ << "(";
    interleaved_for_each(
        *fta.parameters,
        [&](const auto& parameter) { parameter->Accept(*this); },
        [&](const auto& parameter) { out_ << ", "; }
    );
    out_ << ") -> ";
    fta.return_type->Accept(*this);
}

void AstPrinter::Visit(const MethodTypeAnnotation& mta)
{
    out_ << "method ";
    mta.function_type->Accept(*this);
}

void AstPrinter::Visit(const StructExpression& struct_expression)
{
    out_ << "struct";
    out_ << "\n{\n";
    ++indent_;
    for (const auto& statement : *struct_expression.members)
    {
        Print(*statement);
    }
    --indent_;
    out_ << "}";
}

void AstPrinter::PrintQualifier(TypeAnnotationQualifier qualifier)
{
    switch (qualifier)
    {
        case TypeAnnotationQualifier::None:
        {
            return;
        }
        case TypeAnnotationQualifier::Mutable:
        {
            out_ << "mut ";
            return;
        }
        case TypeAnnotationQualifier::Constant:
        {
            out_ << "const ";
            return;
        }
    }
}

std::string str(UnaryOp::Operator op)
{
    switch (op)
    {
        case UnaryOp::Operator::Plus:
            return "+";
        case UnaryOp::Operator::Minus:
            return "-";
        case UnaryOp::Operator::Bang:
            return "!";
        case UnaryOp::Operator::Ampersand:
            return "&";
        case UnaryOp::Operator::Asterisk:
            return "*";
    }
}

std::string str(BinaryOp::Operator op)
{
    switch (op)
    {
        case BinaryOp::Operator::Plus:
            return "+";
        case BinaryOp::Operator::Minus:
            return "-";
        case BinaryOp::Operator::Asterisk:
            return "*";
        case l0::BinaryOp::Operator::AmpersandAmpersand:
            return "&&";
        case l0::BinaryOp::Operator::PipePipe:
            return "||";
        case l0::BinaryOp::Operator::EqualsEquals:
            return "==";
        case l0::BinaryOp::Operator::BangEquals:
            return "!=";
    }
}

}  // namespace l0
