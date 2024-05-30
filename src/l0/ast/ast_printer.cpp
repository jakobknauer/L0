#include "l0/ast/ast_printer.h"

namespace l0
{

AstPrinter::AstPrinter(std::ostream& out) : out_{out} {}

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
    for (const auto& statement : *conditional_statement.then_block)
    {
        Print(*statement);
    }
    out_ << "}\n";

    if (!conditional_statement.else_block)
    {
        return;
    }

    out_ << "else\n";
    out_ << "{\n";
    for (const auto& statement : *conditional_statement.else_block)
    {
        Print(*statement);
    }
    out_ << "}\n";
}

void AstPrinter::Visit(const WhileLoop& while_loop)
{
    out_ << "while ";
    while_loop.condition->Accept(*this);
    out_ << ":\n";

    out_ << "{\n";
    for (const auto& statement : *while_loop.body)
    {
        Print(*statement);
    }
    out_ << "}\n";
}

void AstPrinter::Visit(const Assignment& assignment)
{
    out_ << assignment.variable << " = ";
    assignment.expression->Accept(*this);
}

void AstPrinter::Visit(const BinaryOp& binary_op)
{
    out_ << "(";
    binary_op.left->Accept(*this);

    std::string op;
    switch (binary_op.op)
    {
        case BinaryOp::Operator::Plus:
            op = "+";
            break;
        case BinaryOp::Operator::Minus:
            op = "-";
            break;
        case BinaryOp::Operator::Asterisk:
            op = "*";
            break;
        case l0::BinaryOp::Operator::AmpersandAmpersand:
            op = "&&";
            break;
        case l0::BinaryOp::Operator::PipePipe:
            op = "||";
            break;
        case l0::BinaryOp::Operator::EqualsEquals:
            op = "==";
            break;
        case l0::BinaryOp::Operator::BangEquals:
            op = "!=";
            break;
    }
    out_ << " " << op << " ";

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
    for (auto& statement : *function.statements)
    {
        statement->Accept(*this);
        out_ << ";\n";
    }
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

}  // namespace l0
