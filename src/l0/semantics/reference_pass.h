#ifndef L0_SEMANTICS_REFERENCE_PASS
#define L0_SEMANTICS_REFERENCE_PASS

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/ast/type_expression.h"

namespace l0
{

/// @brief Handles aspects reference semantics that go beyond the type system.
///
/// This consists of asserting the following two properties pertaining lvalues:
///     1) The left side of an assignment must be an lvalue.
///     2) The operand of the unary '&' operator must be an lvalue.
///
/// The follow expressions are lvalues:
///     - Variables
///     - Dereferenced references
///     - Member accessors
class ReferencePass : private IStatementVisitor, private IExpressionVisitor, private ITypeExpressionVisitor
{
   public:
    ReferencePass(Module& module);
    void Run();

   private:
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

    bool IsLValue(std::shared_ptr<Expression> value) const;

    Module& module_;
};

}  // namespace l0

#endif
