#ifndef L0_AST_AST_PRINTER_H
#define L0_AST_AST_PRINTER_H

#include "l0/ast/expression.h"
#include "l0/ast/indent.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"

namespace l0
{

class AstPrinter : IConstExpressionVisitor, IConstStatementVisitor, ITypeAnnotationVisitor
{
   public:
    AstPrinter(std::ostream& out);

    void Print(Module& module);
    void Print(Statement& statement);
    void Print(Expression& expression);

   private:
    void Visit(const Declaration& declaration) override;
    void Visit(const ExpressionStatement& expression_statement) override;
    void Visit(const ReturnStatement& return_statement) override;
    void Visit(const ConditionalStatement& conditional_statement) override;
    void Visit(const WhileLoop& while_loop) override;

    void Visit(const Assignment& assignment) override;
    void Visit(const BinaryOp& binary_op) override;
    void Visit(const Variable& variable) override;
    void Visit(const Call& call) override;
    void Visit(const UnitLiteral& literal) override;
    void Visit(const BooleanLiteral& literal) override;
    void Visit(const IntegerLiteral& literal) override;
    void Visit(const StringLiteral& literal) override;
    void Visit(const Function& function) override;

    void Visit(const SimpleTypeAnnotation& sta) override;
    void Visit(const FunctionTypeAnnotation& fta) override;

    std::ostream& out_;
    detail::Indent indent_;
};

std::string str(BinaryOp::Operator op);

}  // namespace l0

#endif
