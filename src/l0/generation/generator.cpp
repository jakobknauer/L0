#include "l0/generation/generator.h"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

#include <format>

#include "l0/generation/generator_error.h"

namespace l0
{

namespace
{

std::string GetLambdaName()
{
    static int i = 0;
    return std::format("__lambda{}", i++);
}

}  // namespace

Generator::Generator(Module& module)
    : ast_module_{module}, builder_{context_}, llvm_module_{module.name, context_}, type_converter_{context_}
{
}

std::string Generator::Generate()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm_module_.setTargetTriple("x86_64-pc-linux-gnu");

    DeclareExternals();
    DeclareGlobals();
    DefineGlobals();

    std::string code{};
    llvm::raw_string_ostream os{code};
    os << llvm_module_;
    return code;
}

void Generator::DeclareExternals()
{
    for (const std::string& external_symbol : ast_module_.externals->GetVariables())
    {
        const Type& type = *ast_module_.externals->GetType(external_symbol);

        if (auto function_type = dynamic_cast<const FunctionType*>(&type))
        {
            auto llvm_type = type_converter_.Convert(*function_type);
            llvm::FunctionCallee function_callee = llvm_module_.getOrInsertFunction(external_symbol, llvm_type);
            ast_module_.externals->SetLLVMValue(external_symbol, function_callee.getCallee());
        }
        else
        {
            auto llvm_type = type_converter_.Convert(type);
            llvm_module_.getOrInsertGlobal(external_symbol, llvm_type);
            auto global = llvm_module_.getNamedGlobal(external_symbol);
            ast_module_.externals->SetLLVMValue(external_symbol, global);
        }
    }
}

void Generator::DeclareGlobals()
{
    for (const auto& statement : *ast_module_.statements)
    {
        auto* declaration = dynamic_cast<Declaration*>(statement.get());
        const Type& type = *declaration->scope->GetType(declaration->variable);

        if (auto ft = dynamic_cast<const FunctionType*>(&type))
        {
            auto llvm_type = type_converter_.GetDeclarationType(*ft);
            llvm::FunctionCallee function_callee = llvm_module_.getOrInsertFunction(declaration->variable, llvm_type);

            declaration->scope->SetLLVMValue(declaration->variable, function_callee.getCallee());
        }
        else if (dynamic_cast<const StringType*>(&type))
        {
            auto literal = dynamic_cast<StringLiteral*>(declaration->initializer.get());
            auto global = builder_.CreateGlobalStringPtr(literal->value, declaration->variable, 0, &llvm_module_);

            declaration->scope->SetLLVMValue(declaration->variable, global);
        }
        else if (dynamic_cast<const IntegerType*>(&type))
        {
            auto literal = dynamic_cast<IntegerLiteral*>(declaration->initializer.get());
            auto llvm_type = type_converter_.Convert(type);

            llvm_module_.getOrInsertGlobal(declaration->variable, llvm_type);
            auto global = llvm_module_.getNamedGlobal(declaration->variable);
            global->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
            global->setInitializer(llvm::ConstantInt::get(llvm_type, literal->value));
            global->setAlignment(llvm::Align(1));
            global->setConstant(true);

            declaration->scope->SetLLVMValue(declaration->variable, global);
        }
        else
        {
            throw GeneratorError(
                std::format("Unexpected type for global variable '{}': '{}'.", declaration->variable, type.ToString())
            );
        }
    }
}

void Generator::DefineGlobals()
{
    for (const auto& statement : *ast_module_.statements)
    {
        auto* declaration = dynamic_cast<Declaration*>(statement.get());
        const Type& type = *declaration->scope->GetType(declaration->variable);

        auto function_type = dynamic_cast<const FunctionType*>(&type);
        if (!function_type)
        {
            continue;
        }

        Function& function = dynamic_cast<Function&>(*declaration->initializer);
        llvm::Type* llvm_type = type_converter_.Convert(*function_type);

        llvm::FunctionCallee callee = llvm_module_.getOrInsertFunction(declaration->variable, llvm_type);
        llvm::Function* llvm_function = llvm::dyn_cast<llvm::Function>(callee.getCallee());

        llvm::BasicBlock* block =
            llvm::BasicBlock::Create(context_, "entry", llvm::dyn_cast<llvm::Function>(callee.getCallee()));
        builder_.SetInsertPoint(block);

        GenerateFunctionBody(function, *llvm_function);
    }
}

void Generator::Visit(const Declaration& declaration)
{
    declaration.initializer->Accept(*this);
    llvm::Value* initializer = result_;

    const Type& type = *declaration.scope->GetType(declaration.variable);
    llvm::Type* llvm_type = type_converter_.Convert(type);

    if (llvm::isa<llvm::FunctionType>(llvm_type))
    {
        llvm_type = llvm::PointerType::getUnqual(context_);
    }

    llvm::AllocaInst* alloca = builder_.CreateAlloca(llvm_type, nullptr, declaration.variable);
    declaration.scope->SetLLVMValue(declaration.variable, alloca);
    builder_.CreateStore(initializer, alloca);
    result_ = nullptr;
}

void Generator::Visit(const ExpressionStatement& expression_statement)
{
    expression_statement.expression->Accept(*this);
    result_ = nullptr;
}

void Generator::Visit(const ReturnStatement& return_statement)
{
    return_statement.value->Accept(*this);
    auto return_value = result_;

    builder_.CreateRet(return_value);
    result_ = nullptr;
}

void Generator::Visit(const ConditionalStatement& conditional_statement)
{
    conditional_statement.condition->Accept(*this);
    auto condition = result_;

    bool else_exists{conditional_statement.else_block};
    bool merge_needed = !conditional_statement.then_block_returns || !conditional_statement.else_block_returns;

    llvm::Function* llvm_function = builder_.GetInsertBlock()->getParent();

    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(context_, "if", llvm_function);
    llvm::BasicBlock* merge_block = merge_needed ? llvm::BasicBlock::Create(context_, "ifcont") : nullptr;
    llvm::BasicBlock* else_block = else_exists ? llvm::BasicBlock::Create(context_, "else") : nullptr;

    // if
    builder_.CreateCondBr(condition, then_block, else_exists ? else_block : merge_block);

    // then
    builder_.SetInsertPoint(then_block);
    for (const auto& statement : *conditional_statement.then_block)
    {
        statement->Accept(*this);
    }
    if (!conditional_statement.then_block_returns)
    {
        builder_.CreateBr(merge_block);
    }

    // else
    if (else_exists)
    {
        llvm_function->insert(llvm_function->end(), else_block);
        builder_.SetInsertPoint(else_block);
        for (const auto& statement : *conditional_statement.else_block)
        {
            statement->Accept(*this);
        }
        if (!conditional_statement.else_block_returns)
        {
            builder_.CreateBr(merge_block);
        }
    }

    // merge
    if (merge_needed)
    {
        llvm_function->insert(llvm_function->end(), merge_block);
        builder_.SetInsertPoint(merge_block);
    }

    result_ = nullptr;
}

void Generator::Visit(const WhileLoop& while_loop)
{
    llvm::Function* llvm_function = builder_.GetInsertBlock()->getParent();
    // llvm::BasicBlock* preheader = builder_.GetInsertBlock();
    llvm::BasicBlock* header = llvm::BasicBlock::Create(context_, "loopheader", llvm_function);
    llvm::BasicBlock* body = llvm::BasicBlock::Create(context_, "loopbody");
    llvm::BasicBlock* afterloop = llvm::BasicBlock::Create(context_, "afterloop");

    // condition
    builder_.CreateBr(header);
    builder_.SetInsertPoint(header);
    while_loop.condition->Accept(*this);
    auto condition = result_;
    builder_.CreateCondBr(condition, body, afterloop);

    // body
    llvm_function->insert(llvm_function->end(), body);
    builder_.SetInsertPoint(body);
    for (const auto& statement : *while_loop.body)
    {
        statement->Accept(*this);
    }
    builder_.CreateBr(header);

    // afterloop
    llvm_function->insert(llvm_function->end(), afterloop);
    builder_.SetInsertPoint(afterloop);

    result_ = nullptr;
}

void Generator::Visit(const Deallocation& deallocation)
{
    deallocation.reference->Accept(*this);
    auto operand = result_;

    llvm::Type* llvm_ptr_type = llvm::PointerType::get(context_, 0);
    auto llvm_void_type = llvm::Type::getVoidTy(context_);
    auto ptr_to_void = llvm::FunctionType::get(llvm_void_type, llvm_ptr_type, false);
    llvm::FunctionCallee free_function = llvm_module_.getOrInsertFunction("free", ptr_to_void);

    std::vector<llvm::Value*> arguments{};
    arguments.push_back(operand);

    result_ = builder_.CreateCall(ptr_to_void, free_function.getCallee(), arguments);
}

void Generator::Visit(const Assignment& assignment)
{
    llvm::Value* target;
    if (auto variable = dynamic_cast<Variable*>(assignment.target.get()))
    {
        target = variable->scope->GetLLVMValue(variable->name);
    }
    else if (auto unary_op = dynamic_cast<UnaryOp*>(assignment.target.get()))
    {
        if (unary_op->op == UnaryOp::Operator::Asterisk)
        {
            unary_op->operand->Accept(*this);
            target = result_;
        }
        else
        {
            throw GeneratorError("Cannot assign to expression.");
        }
    }
    else
    {
        throw GeneratorError("Cannot assign to expression.");
    }

    assignment.expression->Accept(*this);
    auto value = result_;

    builder_.CreateStore(value, target);
    // leave result_ as it is :)
}

void Generator::Visit(const UnaryOp& unary_op)
{
    switch (unary_op.op)
    {
        case UnaryOp::Operator::Plus:
        {
            unary_op.operand->Accept(*this);
            llvm::Value* operand = result_;
            result_ = operand;
            break;
        }
        case l0::UnaryOp::Operator::Minus:
        {
            unary_op.operand->Accept(*this);
            llvm::Value* operand = result_;
            result_ = builder_.CreateNeg(operand, "negtmp");
            break;
        }
        case l0::UnaryOp::Operator::Bang:
        {
            unary_op.operand->Accept(*this);
            llvm::Value* operand = result_;
            result_ = builder_.CreateNot(operand, "nottmp");
            break;
        }
        case l0::UnaryOp::Operator::Ampersand:
        {
            const Variable* variable = dynamic_cast<const Variable*>(unary_op.operand.get());
            if (!variable)
            {
                throw GeneratorError("Operand of unary operator '&' is not a variable.");
            }
            result_ = variable->scope->GetLLVMValue(variable->name);
            break;
        }
        case l0::UnaryOp::Operator::Asterisk:
        {
            unary_op.operand->Accept(*this);
            result_ = builder_.CreateLoad(type_converter_.Convert(*unary_op.type), result_, "dereftmp");
        }
    }
}

void Generator::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    llvm::Value* left = result_;

    binary_op.right->Accept(*this);
    llvm::Value* right = result_;

    switch (binary_op.op)
    {
        case BinaryOp::Operator::Plus:
        {
            if (auto reference_type = dynamic_cast<ReferenceType*>(binary_op.left->type.get()))
            {
                auto llvm_type = type_converter_.Convert(*binary_op.type);
                std::vector<llvm::Value*> indices = {
                    // llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), 0),
                    right,
                };
                result_ = builder_.CreateGEP(llvm_type, left, indices, "geptmp");
            }
            else
            {
                result_ = builder_.CreateAdd(left, right, "addtmp");
            }
            break;
        }
        case BinaryOp::Operator::Minus:
            result_ = builder_.CreateSub(left, right, "subtmp");
            break;
        case BinaryOp::Operator::Asterisk:
            result_ = builder_.CreateMul(left, right, "multmp");
            break;
        case BinaryOp::Operator::AmpersandAmpersand:
            result_ = builder_.CreateLogicalAnd(left, right, "andtmp");
            break;
        case BinaryOp::Operator::PipePipe:
            result_ = builder_.CreateLogicalOr(left, right, "ortmp");
            break;
        case BinaryOp::Operator::EqualsEquals:
            result_ = builder_.CreateICmpEQ(left, right, "eqtmp");
            break;
        case BinaryOp::Operator::BangEquals:
            result_ = builder_.CreateICmpNE(left, right, "netmp");
            break;
    }
}

void Generator::Visit(const Variable& variable)
{
    llvm::Value* llvm_value = variable.scope->GetLLVMValue(variable.name);
    if (auto allocation = llvm::dyn_cast<llvm::AllocaInst>(llvm_value))
    {
        auto allocated_type = allocation->getAllocatedType();
        result_ = builder_.CreateLoad(allocated_type, llvm_value, variable.name);
    }
    else if (auto global_variable = llvm::dyn_cast<llvm::GlobalVariable>(llvm_value))
    {
        auto llvm_type = global_variable->getValueType();
        if (llvm::isa<llvm::ArrayType>(llvm_type))
        {
            result_ = llvm_value;
        }
        else
        {
            result_ = builder_.CreateLoad(llvm_type, llvm_value, variable.name);
        }
    }
    else if (llvm::isa<llvm::Function>(llvm_value))
    {
        result_ = llvm_value;
    }
    else if (llvm::isa<llvm::Argument>(llvm_value))
    {
        result_ = llvm_value;
    }
    else
    {
        throw GeneratorError(std::format("Unexpected LLVM value stored for variable '{}'.", variable.name));
    }
}

void Generator::Visit(const Call& call)
{
    call.function->Accept(*this);
    llvm::Value* llvm_function = result_;

    FunctionType* function_type = dynamic_cast<FunctionType*>(call.function->type.get());
    llvm::FunctionType* llvm_function_type = type_converter_.GetDeclarationType(*function_type);

    std::vector<llvm::Value*> arguments{};
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
        arguments.push_back(result_);
    }

    result_ = builder_.CreateCall(llvm_function_type, llvm_function, arguments, "calltmp");
}

void Generator::Visit(const UnitLiteral& literal)
{
    auto unit_type = llvm::dyn_cast<llvm::StructType>(type_converter_.Convert(*literal.type));
    result_ = llvm::ConstantStruct::get(unit_type, {});
}

void Generator::Visit(const BooleanLiteral& literal)
{
    llvm::Type* type = llvm::Type::getInt1Ty(context_);
    result_ = llvm::ConstantInt::get(type, literal.value ? 1 : 0);
}

void Generator::Visit(const IntegerLiteral& literal)
{
    llvm::Type* type = llvm::Type::getInt64Ty(context_);
    result_ = llvm::ConstantInt::get(type, literal.value);
}

void Generator::Visit(const StringLiteral& literal) { result_ = builder_.CreateGlobalString(literal.value); }

void Generator::Visit(const Function& function)
{
    llvm::BasicBlock* previous_block = builder_.GetInsertBlock();

    auto llvm_type = type_converter_.GetDeclarationType(dynamic_cast<FunctionType&>(*function.type));
    llvm::FunctionCallee callee = llvm_module_.getOrInsertFunction(GetLambdaName(), llvm_type);
    llvm::Function* llvm_function = llvm::dyn_cast<llvm::Function>(callee.getCallee());

    llvm::BasicBlock* block = llvm::BasicBlock::Create(context_, "entry", dyn_cast<llvm::Function>(callee.getCallee()));
    builder_.SetInsertPoint(block);

    GenerateFunctionBody(function, *llvm_function);

    result_ = llvm_function;

    builder_.SetInsertPoint(previous_block);
}

void Generator::Visit(const Allocation& allocation)
{
    llvm::Type* llvm_int_type = type_converter_.Convert(IntegerType());
    llvm::Type* llvm_ptr_type = llvm::PointerType::get(context_, 0);
    llvm::FunctionType* int_to_ptr = llvm::FunctionType::get(llvm_ptr_type, llvm_int_type, false);
    llvm::FunctionCallee malloc_function = llvm_module_.getOrInsertFunction("malloc", int_to_ptr);

    llvm::Type* allocated_llvm_type = type_converter_.Convert(*allocation.allocated_type);
    auto type_size = llvm::ConstantInt::get(llvm_int_type, data_layout_.getTypeAllocSize(allocated_llvm_type));
    std::vector<llvm::Value*> arguments{};
    if (allocation.size)
    {
        allocation.size->Accept(*this);
        auto size = result_;
        auto total_size = builder_.CreateMul(type_size, size);
        arguments.push_back(total_size);
    }
    else
    {
        arguments.push_back(type_size);
    }

    result_ = builder_.CreateCall(int_to_ptr, malloc_function.getCallee(), arguments, "allocation");
}

void Generator::GenerateFunctionBody(const Function& function, llvm::Function& llvm_function)
{
    for (std::size_t i = 0; i < function.parameters->size(); ++i)
    {
        ParameterDeclaration& param = *function.parameters->at(i);
        llvm::Argument* llvm_param = llvm_function.args().begin() + i;
        llvm_param->setName(param.name);
        function.locals->SetLLVMValue(param.name, llvm_param);
    }

    for (const auto& statement : *function.statements)
    {
        statement->Accept(*this);
    }

    llvm::verifyFunction(llvm_function);
}

}  // namespace l0
