#ifndef L0_SEMANTICS_TYPECHECKER_H
#define L0_SEMANTICS_TYPECHECKER_H

#include <memory>
#include <unordered_map>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/semantics/operator_overload_resolver.h"
#include "l0/semantics/type_annotation_converter.h"
#include "l0/types/types.h"

namespace l0
{

class Typechecker : private IConstExpressionVisitor, private IConstStatementVisitor
{
   public:
    Typechecker(Module& module);

    void Check();

   private:
    Module& module_;
    std::unordered_map<std::string, const std::shared_ptr<Type>> simple_types_{};
    TypeAnnotationConverter converter_{};
    detail::OperatorOverloadResolver operator_overload_resolver_{};

    void Visit(const Declaration& declaration) override;
    void Visit(const ExpressionStatement& expression_statement) override;
    void Visit(const ReturnStatement& return_statement) override;
    void Visit(const ConditionalStatement& conditional_statement) override;
    void Visit(const WhileLoop& while_loop) override;
    void Visit(const Deallocation& declaration) override;

    void Visit(const Assignment& assignment) override;
    void Visit(const UnaryOp& unary_op) override;
    void Visit(const BinaryOp& binary_op) override;
    void Visit(const Variable& variable) override;
    void Visit(const Call& call) override;
    void Visit(const UnitLiteral& literal) override;
    void Visit(const BooleanLiteral& literal) override;
    void Visit(const IntegerLiteral& literal) override;
    void Visit(const StringLiteral& literal) override;
    void Visit(const Function& function) override;
    void Visit(const Allocation& allocation) override;
};

}  // namespace l0

#endif
