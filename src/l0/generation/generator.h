#ifndef L0_GENERATION_GENERATOR_H
#define L0_GENERATION_GENERATOR_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <string>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/generation/type_converter.h"

namespace l0
{

class Generator : private IConstExpressionVisitor, IConstStatementVisitor
{
   public:
    Generator(llvm::LLVMContext& context, Module& module);

    std::string Generate();

   private:
    Module& ast_module_;

    llvm::LLVMContext& context_;
    llvm::IRBuilder<> builder_;
    llvm::Module llvm_module_;
    llvm::DataLayout data_layout_{&llvm_module_};

    TypeConverter type_converter_;
    llvm::Value* result_;
    llvm::Value* result_address_;
    llvm::Value* object_ptr_;

    void DeclareExternalVariables();
    void DeclareTypes();
    void FillTypes();
    void DeclareCallables();
    void DeclareCallable(std::shared_ptr<Function> function);
    void DefineCallables();

    void Visit(const StatementBlock& statement_block) override;
    void Visit(const Declaration& declaration) override;
    void Visit(const TypeDeclaration& declaration) override;
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
    void Visit(const Initializer& Initializer) override;
    void Visit(const Allocation& allocation) override;

    void GenerateFunctionBody(const Function& function, llvm::Function& llvm_function);
    llvm::AllocaInst* GenerateAlloca(llvm::Type* type, std::string name);
    void GenerateResultAddress();

    std::vector<std::tuple<std::string, llvm::Value*>> GetActualMemberInitializers(
        const MemberInitializerList& explicit_initializers, const StructType& struct_type
    );
};

}  // namespace l0

#endif
