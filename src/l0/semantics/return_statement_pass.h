#ifndef L0_SEMANTICS_RETURN_STATEMENT_PASS_H
#define L0_SEMANTICS_RETURN_STATEMENT_PASS_H

#include <stack>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/types/types.h"

namespace l0
{

class ReturnStatementPass : private IConstStatementVisitor, private IConstExpressionVisitor
{
   public:
    ReturnStatementPass(Module& module);
    void Run();

   private:
    Module& module_;

    bool always_returns_{false};
    std::stack<Type*> expected_return_value_{};

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

    void Visit(const StatementBlock& statement_block);
};

}  // namespace l0

#endif
