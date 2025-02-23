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

namespace l0::detail
{

class Generator : private IConstExpressionVisitor, IConstStatementVisitor
{
   public:
    Generator(llvm::LLVMContext& context, Module& module);

    void Run();

   private:
    Module& ast_module_;

    llvm::LLVMContext& context_;
    llvm::IRBuilder<> builder_;
    llvm::Module* llvm_module_;
    llvm::DataLayout data_layout_{llvm_module_};

    llvm::StructType* closure_type_;
    llvm::PointerType* pointer_type_;
    llvm::Type* int_type_;
    llvm::Type* char_type_;
    llvm::Type* bool_type_;

    TypeConverter type_converter_;

    void DeclareTypes();
    void DeclareEnvironmentVariables();
    void DeclareExternalVariables();
    void DeclareGlobalVariables();
    void DeclareCallables();
    void DeclareCallable(std::shared_ptr<Function> function);

    void DefineTypes();
    void DefineStructType(const StructType& type);
    void DefineEnumType(const EnumType& type);
    void DefineGlobalVariables();
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

    void GenerateFunctionBody(
        const Function& function, llvm::Function& llvm_function, llvm::StructType* context_struct = nullptr
    );

    std::vector<std::tuple<std::string, llvm::Value*>> GetActualMemberInitializers(
        const MemberInitializerList& explicit_initializers, const StructType& struct_type, const Scope& scope
    );

    llvm::StructType* GenerateClosureContextStruct(const Function& function);
    std::tuple<llvm::Value*, llvm::StructType*> GenerateClosureContext(const Function& function);
    void VisitGlobal(llvm::GlobalVariable* global_variable);

    llvm::Value* GenerateMallocCall(llvm::Value* size, const std::string& name);

    class ResultStore
    {
       public:
        ResultStore(llvm::IRBuilder<>& builder);

        llvm::Value* GetResult();
        llvm::Value* GetResultAddress();
        llvm::Value* GetObjectPointer();
        bool HasObjectPointer();

        void Clear();
        void SetResult(llvm::Value* result);
        void SetResultAndObjectPointer(llvm::Value* result, llvm::Value* object_ptr);
        void SetResultAddress(llvm::Value* result_address, llvm::Type* result_type);
        void SetResultAddressAndObjectPointer(
            llvm::Value* result_address, llvm::Type* result_type, llvm::Value* object_ptr
        );
        void SetResultAndResultAddress(llvm::Value* result, llvm::Value* result_address);
        void SetObjectPointerNoOverride(llvm::Value* object_ptr);

       private:
        llvm::IRBuilder<>& builder_;

        llvm::Value* result_{nullptr};
        llvm::Value* result_address_{nullptr};
        llvm::Type* result_type_{nullptr};
        llvm::Value* object_ptr_{nullptr};
    };
    ResultStore result_store_{builder_};
};

llvm::AllocaInst* GenerateAlloca(llvm::IRBuilder<>& builder, llvm::Type* type, std::string name);

}  // namespace l0::detail

#endif
