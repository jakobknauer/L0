#include "l0/ast/ast_printer.h"

namespace l0
{

AstPrinter::AstPrinter(std::ostream& out) : out_{out}, indent_{out_} {}

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

void AstPrinter::Print(Expression& expression) { expression.Accept(*this); }

void AstPrinter::Visit(const Declaration& declaration)
{
    out_ << declaration.variable << ": ";
    declaration.annotation->Accept(*this);
    out_ << " = ";
    declaration.initializer->Accept(*this);
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

void AstPrinter::Visit(const Assignment& assignment)
{
    assignment.target->Accept(*this);
    out_ << " = ";
    assignment.expression->Accept(*this);
}

void AstPrinter::Visit(const UnaryOp& unary_op)
{
    out_ << str(unary_op.op) << "(";
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

void AstPrinter::Visit(const Variable& variable) { out_ << variable.name; }

void AstPrinter::Visit(const Call& call)
{
    call.function->Accept(*this);
    out_ << "(";
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
        out_ << ", ";
    }
    out_ << ")";
}

void AstPrinter::Visit(const UnitLiteral& literal) { out_ << "unit"; }

void AstPrinter::Visit(const BooleanLiteral& literal) { out_ << (literal.value ? "true" : "false"); }

void AstPrinter::Visit(const IntegerLiteral& literal) { out_ << literal.value; }

void AstPrinter::Visit(const StringLiteral& literal) { out_ << "\"" << literal.value << "\""; }

void AstPrinter::Visit(const Function& function)
{
    out_ << "$(";
    for (auto& parameter : *function.parameters)
    {
        out_ << parameter->name << ": ";
        parameter->annotation->Accept(*this);
        out_ << ", ";
    }
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

void AstPrinter::Visit(const SimpleTypeAnnotation& sta) { out_ << sta.type; }

void AstPrinter::Visit(const FunctionTypeAnnotation& fta)
{
    out_ << "(";
    for (auto& parameter : *fta.parameters)
    {
        parameter->Accept(*this);
        out_ << ", ";
    }
    out_ << ") -> ";
    fta.return_type->Accept(*this);
}

void AstPrinter::Visit(const ReferenceTypeAnnotation& rta)
{
    out_ << "&";
    rta.base_type->Accept(*this);
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
