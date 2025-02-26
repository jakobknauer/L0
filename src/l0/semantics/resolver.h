#ifndef L0_SEMANTICS_DECLARATION_CHECKER_H
#define L0_SEMANTICS_DECLARATION_CHECKER_H

#include <memory>
#include <optional>
#include <stack>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/scope.h"
#include "l0/ast/statement.h"

namespace l0::detail
{

class Resolver : private IConstExpressionVisitor, private IConstStatementVisitor, private IConstTypeExpressionVisitor
{
   public:
    Resolver(const Module& module);

    void Run();

   private:
    const Module& module_;
    std::vector<std::shared_ptr<Scope>> scopes_{};
    std::stack<Identifier> namespaces_{};

    void Visit(const StatementBlock& statement_block) override;
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
    void Visit(const CharacterLiteral& literal) override;
    void Visit(const StringLiteral& literal) override;
    void Visit(const Function& function) override;
    void Visit(const Initializer& initializer) override;
    void Visit(const Allocation& allocation) override;

    void Visit(const StructExpression& struct_expression) override;
    void Visit(const EnumExpression& enum_expression) override;

    std::optional<std::shared_ptr<Scope>> Resolve(const Identifier& identifier);
    std::pair<std::shared_ptr<Scope>, Identifier> Resolve(const Identifier& identifier, const Identifier& namespace_);
};

}  // namespace l0::detail

#endif
