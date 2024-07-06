#ifndef L0_SEMANTICS_RETURN_STATEMENT_PASS_H
#define L0_SEMANTICS_RETURN_STATEMENT_PASS_H

#include <stack>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/types/types.h"

namespace l0
{

class ReturnStatementPass : private IStatementVisitor, private IExpressionVisitor
{
   public:
    ReturnStatementPass(Module& module);
    void Run();

   private:
    Module& module_;

    bool statement_returns_{false};
    std::stack<Type*> expected_return_value_{};

    void Visit(Declaration& declaration) override;
    void Visit(ExpressionStatement& expression_statement) override;
    void Visit(ReturnStatement& return_statement) override;
    void Visit(ConditionalStatement& conditional_statement) override;
    void Visit(WhileLoop& while_loop) override;

    void Visit(Assignment& assignment) override;
    void Visit(UnaryOp& unary_op) override;
    void Visit(BinaryOp& binary_op) override;
    void Visit(Variable& variable) override;
    void Visit(Call& call) override;
    void Visit(UnitLiteral& literal) override;
    void Visit(BooleanLiteral& literal) override;
    void Visit(IntegerLiteral& literal) override;
    void Visit(StringLiteral& literal) override;
    void Visit(Function& function) override;
    void Visit(Allocation& allocation) override;

    void Visit(StatementBlock& statement_block);
};

}  // namespace l0

#endif
