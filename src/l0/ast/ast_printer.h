#ifndef L0_AST_AST_PRINTER_H
#define L0_AST_AST_PRINTER_H

#include "l0/ast/expression.h"
#include "l0/ast/indent.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"
#include "l0/ast/type_expression.h"

namespace l0
{

class AstPrinter : IConstExpressionVisitor, IConstStatementVisitor, ITypeAnnotationVisitor, IConstTypeExpressionVisitor
{
   public:
    AstPrinter(std::ostream& out);

    void Print(Module& module);
    void Print(Statement& statement);
    void Print(Expression& expression);

   private:
    void Visit(const Declaration& declaration) override;
    void Visit(const TypeDeclaration& type_declaration) override;
    void Visit(const ExpressionStatement& expression_statement) override;
    void Visit(const ReturnStatement& return_statement) override;
    void Visit(const ConditionalStatement& conditional_statement) override;
    void Visit(const WhileLoop& while_loop) override;
    void Visit(const Deallocation& deallocation) override;

    void Visit(const Assignment& assignment) override;
    void Visit(const UnaryOp& unary_op) override;
    void Visit(const BinaryOp& binary_op) override;
    void Visit(const Variable& variable) override;
    void Visit(const MemberAccessor& member_accessor) override;
    void Visit(const Call& call) override;
    void Visit(const UnitLiteral& literal) override;
    void Visit(const BooleanLiteral& literal) override;
    void Visit(const IntegerLiteral& literal) override;
    void Visit(const StringLiteral& literal) override;
    void Visit(const Function& function) override;
    void Visit(const Initializer& initializer) override;
    void Visit(const Allocation& allocation) override;

    void Visit(const SimpleTypeAnnotation& sta) override;
    void Visit(const ReferenceTypeAnnotation& rta) override;
    void Visit(const FunctionTypeAnnotation& fta) override;

    void Visit(const StructExpression& struct_expression) override;

    void PrintQualifier(TypeAnnotationQualifier qualifier);

    std::ostream& out_;
    detail::Indent indent_;
};

std::string str(UnaryOp::Operator op);
std::string str(BinaryOp::Operator op);

}  // namespace l0

#endif
