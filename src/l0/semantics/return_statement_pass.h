#ifndef L0_SEMANTICS_RETURN_STATEMENT_PASS_H
#define L0_SEMANTICS_RETURN_STATEMENT_PASS_H

#include <stack>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/ast/type_expression.h"
#include "l0/semantics/conversion_checker.h"
#include "l0/semantics/type_resolver.h"
#include "l0/types/types.h"

namespace l0::detail
{

class ReturnStatementPass : private IStatementVisitor, private IExpressionVisitor, private ITypeExpressionVisitor
{
   public:
    ReturnStatementPass(Module& module);
    void Run();

   private:
    Module& module_;
    detail::TypeResolver type_resolver_{module_};
    detail::ConversionChecker conversion_checker_{type_resolver_};

    bool statement_returns_{false};
    std::stack<std::shared_ptr<Type>> expected_return_value_{};

    void Visit(StatementBlock& statement_block) override;
    void Visit(Declaration& declaration) override;
    void Visit(TypeDeclaration& type_declaration) override;
    void Visit(ExpressionStatement& expression_statement) override;
    void Visit(ReturnStatement& return_statement) override;
    void Visit(ConditionalStatement& conditional_statement) override;
    void Visit(WhileLoop& while_loop) override;
    void Visit(Deallocation& deallocation) override;

    void Visit(Assignment& assignment) override;
    void Visit(UnaryOp& unary_op) override;
    void Visit(BinaryOp& binary_op) override;
    void Visit(Variable& variable) override;
    void Visit(MemberAccessor& member_accessor) override;
    void Visit(Call& call) override;
    void Visit(UnitLiteral& literal) override;
    void Visit(BooleanLiteral& literal) override;
    void Visit(IntegerLiteral& literal) override;
    void Visit(CharacterLiteral& literal) override;
    void Visit(StringLiteral& literal) override;
    void Visit(Function& function) override;
    void Visit(Initializer& initializer) override;
    void Visit(Allocation& allocation) override;

    void Visit(StructExpression& struct_expression) override;
    void Visit(EnumExpression& enum_expression) override;
};

}  // namespace l0::detail

#endif
