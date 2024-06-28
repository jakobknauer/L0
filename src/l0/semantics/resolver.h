#ifndef L0_SEMANTICS_DECLARATION_CHECKER_H
#define L0_SEMANTICS_DECLARATION_CHECKER_H

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/scope.h"
#include "l0/ast/statement.h"
#include "l0/semantics/type_annotation_converter.h"

namespace l0
{

class Resolver : IConstExpressionVisitor, IConstStatementVisitor
{
   public:
    Resolver(const Module& module);

    void Check();

   private:
    const Module& module_;
    std::vector<std::shared_ptr<Scope>> scopes_{};
    bool local_{false};
    TypeAnnotationConverter converter_{};

    void Visit(const Declaration& statement) override;
    void Visit(const ExpressionStatement& statement_statement) override;
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

    std::shared_ptr<Scope> Resolve(const std::string name);
};

}  // namespace l0

#endif
