#include "l0/generation/generator.h"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

#include <algorithm>
#include <format>
#include <ranges>
#include <string>

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

constexpr std::string kAllocationBlockName{"allocas"};

}  // namespace

Generator::Generator(Module& module)
    : ast_module_{module},
      builder_{context_},
      llvm_module_{module.name, context_},
      type_converter_{context_}
{
}

std::string Generator::Generate()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm_module_.setTargetTriple("x86_64-pc-linux-gnu");

    DeclareExternals();

    DeclareGlobalTypes();
    FillGlobalTypes();

    DeclareCallables();
    DefineCallables();

    std::string code{};
    llvm::raw_string_ostream os{code};
    os << llvm_module_;
    return code;
}

void Generator::DeclareExternals()
{
    for (const std::string& external_symbol : ast_module_.externals->GetVariables())
    {
        auto type = ast_module_.externals->GetVariableType(external_symbol);

        if (auto function_type = dynamic_pointer_cast<FunctionType>(type))
        {
            auto llvm_type = type_converter_.Convert(*function_type);
            llvm::FunctionCallee function_callee = llvm_module_.getOrInsertFunction(external_symbol, llvm_type);
            ast_module_.externals->SetLLVMValue(external_symbol, function_callee.getCallee());
        }
        else
        {
            auto llvm_type = type_converter_.Convert(*type);
            llvm_module_.getOrInsertGlobal(external_symbol, llvm_type);
            auto global = llvm_module_.getNamedGlobal(external_symbol);
            ast_module_.externals->SetLLVMValue(external_symbol, global);
        }
    }
}

void Generator::DeclareGlobalTypes()
{
    for (const auto& type : ast_module_.globals->GetTypes())
    {
        llvm::StructType::create(context_, type);
    }
}

void Generator::FillGlobalTypes()
{
    for (const auto& type : ast_module_.globals->GetTypes())
    {
        auto struct_type = dynamic_pointer_cast<StructType>(ast_module_.globals->GetTypeDefinition(type));
        if (!struct_type)
        {
            continue;
        }
        auto llvm_struct_type = llvm::StructType::getTypeByName(context_, type);

        std::vector<llvm::Type*> members{};
        for (const auto& member : *struct_type->members)
        {
            members.push_back(type_converter_.GetValueDeclarationType(*member->type));
        }
        llvm_struct_type->setBody(members, true);
    }
}

void Generator::DeclareCallables()
{
    for (auto callable : ast_module_.callables)
    {
        DeclareCallable(callable);
    }
}

void Generator::DeclareCallable(std::shared_ptr<Function> function)
{
    auto type = dynamic_pointer_cast<FunctionType>(function->type);
    if (!type)
    {
        throw GeneratorError(std::format(
            "Unexpected type for global callable '{}': '{}'.", function->global_name.value(), function->type->ToString()
        ));
    }

    auto llvm_type = type_converter_.GetFunctionDeclarationType(*type);
    llvm::FunctionCallee function_callee = llvm_module_.getOrInsertFunction(function->global_name.value(), llvm_type);

    ast_module_.globals->SetLLVMValue(function->global_name.value(), function_callee.getCallee());
}

void Generator::DefineCallables()
{
    for (auto callable : ast_module_.callables)
    {
        auto function_type = dynamic_pointer_cast<FunctionType>(callable->type);
        if (!function_type)
        {
            continue;
        }

        llvm::Type* llvm_type = type_converter_.Convert(*function_type);

        llvm::FunctionCallee callee = llvm_module_.getOrInsertFunction(callable->global_name.value(), llvm_type);
        llvm::Function* llvm_function = llvm::dyn_cast<llvm::Function>(callee.getCallee());

        GenerateFunctionBody(*callable, *llvm_function);
    }
}

void Generator::Visit(const StatementBlock& statement_block)
{
    for (auto statement : statement_block.statements)
    {
        statement->Accept(*this);
    }
}

void Generator::Visit(const Declaration& declaration)
{
    declaration.initializer->Accept(*this);
    llvm::Value* initializer = result_;

    std::shared_ptr<Type> type = declaration.scope->GetVariableType(declaration.variable);
    llvm::Type* llvm_type = type_converter_.GetValueDeclarationType(*type);
    llvm::AllocaInst* alloca = GenerateAlloca(llvm_type, declaration.variable);

    declaration.scope->SetLLVMValue(declaration.variable, alloca);
    builder_.CreateStore(initializer, alloca);

    result_ = nullptr;
}

void Generator::Visit(const TypeDeclaration& type_declaration) {}

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
    conditional_statement.then_block->Accept(*this);
    if (!conditional_statement.then_block_returns)
    {
        builder_.CreateBr(merge_block);
    }

    // else
    if (else_exists)
    {
        llvm_function->insert(llvm_function->end(), else_block);
        builder_.SetInsertPoint(else_block);
        conditional_statement.else_block->Accept(*this);
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
    while_loop.body->Accept(*this);
    builder_.CreateBr(header);

    // afterloop
    llvm_function->insert(llvm_function->end(), afterloop);
    builder_.SetInsertPoint(afterloop);

    result_ = nullptr;
}

void Generator::Visit(const Deallocation& deallocation)
{
    llvm::Type* llvm_ptr_type = llvm::PointerType::get(context_, 0);
    llvm::Type* llvm_void_type = llvm::Type::getVoidTy(context_);
    llvm::FunctionType* ptr_to_void = llvm::FunctionType::get(llvm_void_type, llvm_ptr_type, false);

    llvm::FunctionCallee free_function = llvm_module_.getOrInsertFunction("free", ptr_to_void);

    deallocation.reference->Accept(*this);
    llvm::Value* operand = result_;
    std::vector<llvm::Value*> arguments{operand};

    result_ = builder_.CreateCall(ptr_to_void, free_function.getCallee(), arguments);
}

void Generator::Visit(const Assignment& assignment)
{
    // ReferencePass sets target_address
    assignment.target->Accept(*this);
    GenerateResultAddress();
    auto target_address = result_address_;

    assignment.expression->Accept(*this);
    auto value = result_;

    builder_.CreateStore(value, target_address);
    // leave result_ as it is :)
    result_address_ = target_address;
}

void Generator::Visit(const UnaryOp& unary_op)
{
    switch (unary_op.overload)
    {
        case UnaryOp::Overload::IntegerIdentity:
        {
            unary_op.operand->Accept(*this);
            // leave result_ and result_address
            break;
        }
        case UnaryOp::Overload::IntegerNegation:
        {
            unary_op.operand->Accept(*this);
            llvm::Value* operand = result_;
            result_ = builder_.CreateNeg(operand, "negtmp");
            result_address_ = nullptr;
            break;
        }
        case UnaryOp::Overload::BooleanNegation:
        {
            unary_op.operand->Accept(*this);
            llvm::Value* operand = result_;
            result_ = builder_.CreateNot(operand, "nottmp");
            result_address_ = nullptr;
            break;
        }
        case UnaryOp::Overload::AddressOf:
        {
            unary_op.operand->Accept(*this);
            GenerateResultAddress();
            result_ = result_address_;
            result_address_ = nullptr;
            break;
        }
        case UnaryOp::Overload::Dereferenciation:
        {
            unary_op.operand->Accept(*this);
            auto address = result_;
            auto llvm_type = type_converter_.Convert(*unary_op.type);
            result_ =
                builder_.CreateLoad(llvm_type, address, std::format("deref__{}", std::string{result_->getName()}));
            result_address_ = address;
        }
    }
}

void Generator::Visit(const BinaryOp& binary_op)
{
    binary_op.left->Accept(*this);
    llvm::Value* left = result_;

    binary_op.right->Accept(*this);
    llvm::Value* right = result_;

    switch (binary_op.overload)
    {
        case BinaryOp::Overload::ReferenceIndexation:
        {
            auto reference_type = dynamic_pointer_cast<ReferenceType>(binary_op.left->type);
            auto llvm_type = type_converter_.Convert(*reference_type->base_type);
            std::vector<llvm::Value*> indices = {right};
            result_ = builder_.CreateGEP(llvm_type, left, indices, "indextmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerAddition:
        {
            result_ = builder_.CreateAdd(left, right, "addtmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerSubtraction:
        {
            result_ = builder_.CreateSub(left, right, "subtmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerMultiplication:
        {
            result_ = builder_.CreateMul(left, right, "multmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerDivision:
        {
            result_ = builder_.CreateSDiv(left, right, "sdivtmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerRemainder:
        {
            result_ = builder_.CreateURem(left, right, "uremtmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::BooleanConjunction:
        {
            result_ = builder_.CreateLogicalAnd(left, right, "andtmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::BooleanDisjunction:
        {
            result_ = builder_.CreateLogicalOr(left, right, "ortmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::BooleanEquality:
        case BinaryOp::Overload::IntegerEquality:
        case BinaryOp::Overload::CharacterEquality:
        {
            result_ = builder_.CreateICmpEQ(left, right, "eqtmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::BooleanInequality:
        case BinaryOp::Overload::IntegerInequality:
        case BinaryOp::Overload::CharacterInequality:
        {
            result_ = builder_.CreateICmpNE(left, right, "netmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerLess:
        {
            result_ = builder_.CreateICmpSLT(left, right, "slttmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerGreater:
        {
            result_ = builder_.CreateICmpSGT(left, right, "sgttmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerLessOrEquals:
        {
            result_ = builder_.CreateICmpSLE(left, right, "sletmp");
            result_address_ = nullptr;
            break;
        }
        case BinaryOp::Overload::IntegerGreaterOrEquals:
        {
            result_ = builder_.CreateICmpSGE(left, right, "sgetmp");
            result_address_ = nullptr;
            break;
        }
    }
}

void Generator::Visit(const Variable& variable)
{
    llvm::Value* llvm_value = variable.scope->GetLLVMValue(variable.name);
    if (auto allocation = llvm::dyn_cast<llvm::AllocaInst>(llvm_value))
    {
        auto allocated_type = allocation->getAllocatedType();
        result_ = builder_.CreateLoad(allocated_type, llvm_value, variable.name);
        result_address_ = llvm_value;
    }
    else if (auto global_variable = llvm::dyn_cast<llvm::GlobalVariable>(llvm_value))
    {
        auto llvm_type = global_variable->getValueType();
        if (llvm::isa<llvm::ArrayType>(llvm_type))
        {
            result_ = llvm_value;
            result_address_ = nullptr;
        }
        else
        {
            result_ = builder_.CreateLoad(llvm_type, llvm_value, variable.name);
            result_address_ = llvm_value;
        }
    }
    else if (llvm::isa<llvm::Function>(llvm_value))
    {
        result_ = llvm_value;
        result_address_ = nullptr;
    }
    else if (llvm::isa<llvm::Argument>(llvm_value))
    {
        result_ = llvm_value;
        result_address_ = nullptr;
    }
    else
    {
        throw GeneratorError(std::format("Unexpected LLVM value stored for variable '{}'.", variable.name));
    }
}

void Generator::Visit(const MemberAccessor& member_accessor)
{
    auto struct_name = member_accessor.object_type->name;

    member_accessor.object->Accept(*this);
    GenerateResultAddress();
    auto object_ptr = result_address_;
    auto llvm_struct_type = type_converter_.Convert(*member_accessor.object_type);

    auto member_address = builder_.CreateConstGEP2_32(
        llvm_struct_type,
        object_ptr,
        0,
        member_accessor.member_index,
        std::format("member_address__{}__{}", struct_name, member_accessor.member)
    );

    auto llvm_member_type = type_converter_.GetValueDeclarationType(*member_accessor.type);
    result_ = builder_.CreateLoad(
        llvm_member_type, member_address, std::format("member__{}__{}", struct_name, member_accessor.member)
    );
    result_address_ = member_address;
    object_ptr_ = object_ptr;
}

void Generator::Visit(const Call& call)
{
    call.function->Accept(*this);

    llvm::Value* llvm_function = result_;
    llvm::Value* object_ptr = object_ptr_;
    object_ptr_ = nullptr;

    auto function_type = dynamic_pointer_cast<FunctionType>(call.function->type);
    llvm::FunctionType* llvm_function_type = type_converter_.GetFunctionDeclarationType(*function_type);

    std::vector<llvm::Value*> arguments{};
    if (call.is_method_call)
    {
        arguments.push_back(object_ptr);
    }
    for (auto& argument : *call.arguments)
    {
        argument->Accept(*this);
        arguments.push_back(result_);
    }

    result_ = builder_.CreateCall(llvm_function_type, llvm_function, arguments, "calltmp");
    result_address_ = nullptr;
}

void Generator::Visit(const UnitLiteral& literal)
{
    auto unit_type = llvm::dyn_cast<llvm::StructType>(type_converter_.Convert(*literal.type));
    result_ = llvm::ConstantStruct::get(unit_type, {});
    result_address_ = nullptr;
}

void Generator::Visit(const BooleanLiteral& literal)
{
    llvm::Type* type = llvm::Type::getInt1Ty(context_);
    result_ = llvm::ConstantInt::get(type, literal.value ? 1 : 0);
    result_address_ = nullptr;
}

void Generator::Visit(const IntegerLiteral& literal)
{
    llvm::Type* type = llvm::Type::getInt64Ty(context_);
    result_ = llvm::ConstantInt::get(type, literal.value);
    result_address_ = nullptr;
}

void Generator::Visit(const CharacterLiteral& literal)
{
    llvm::Type* type = llvm::Type::getInt8Ty(context_);
    result_ = llvm::ConstantInt::get(type, literal.value);
    result_address_ = nullptr;
}

void Generator::Visit(const StringLiteral& literal)
{
    result_ = builder_.CreateGlobalString(literal.value);
    result_address_ = nullptr;
}

void Generator::Visit(const Function& function)
{
    if (!function.global_name.has_value())
    {
        function.global_name = GetLambdaName();
    }

    llvm::Function* llvm_function = llvm_module_.getFunction(function.global_name.value());

    if (llvm_function)
    {
        result_ = llvm_function;
        result_address_ = nullptr;
        return;
    }

    auto llvm_type = type_converter_.GetFunctionDeclarationType(*dynamic_pointer_cast<FunctionType>(function.type));
    auto callee = llvm_module_.getOrInsertFunction(function.global_name.value(), llvm_type);
    llvm_function = llvm::dyn_cast<llvm::Function>(callee.getCallee());

    llvm::BasicBlock* previous_block = builder_.GetInsertBlock();

    GenerateFunctionBody(function, *llvm_function);

    builder_.SetInsertPoint(previous_block);

    result_ = llvm_function;
    result_address_ = nullptr;
}

void Generator::Visit(const Initializer& initializer)
{
    auto struct_type = dynamic_pointer_cast<StructType>(initializer.type);
    if (!struct_type)
    {
        throw GeneratorError(
            std::format("Type of initializer must by a StructType, but is of type '{}'.", initializer.type->ToString())
        );
    }

    llvm::Type* llvm_type = type_converter_.Convert(*initializer.type);
    auto alloca = GenerateAlloca(llvm_type, std::format("structinit_address__{}", struct_type->name));

    auto actual_initializers = GetActualMemberInitializers(*initializer.member_initializers, *struct_type);

    for (const auto& [name, init_value] : actual_initializers)
    {
        auto member_index = struct_type->GetMemberIndex(name);
        auto member_address = builder_.CreateConstGEP2_32(
            llvm_type,
            alloca,
            0,
            member_index,
            std::format("structinit_member_address__{}__{}", struct_type->name, name)
        );
        builder_.CreateStore(init_value, member_address);
    }

    result_ = builder_.CreateLoad(llvm_type, alloca, std::format("structinit__{}", struct_type->name));
    result_address_ = alloca;
}

void Generator::Visit(const Allocation& allocation)
{
    llvm::Type* llvm_int_type = type_converter_.Convert(IntegerType{TypeQualifier::Constant});
    llvm::Type* llvm_ptr_type = llvm::PointerType::get(context_, 0);
    llvm::FunctionType* int_to_ptr = llvm::FunctionType::get(llvm_ptr_type, llvm_int_type, false);

    llvm::FunctionCallee malloc_function = llvm_module_.getOrInsertFunction("malloc", int_to_ptr);

    llvm::Value* size_to_allocate;
    llvm::Type* allocated_llvm_type = type_converter_.Convert(*allocation.allocated_type);
    llvm::Value* type_size = llvm::ConstantInt::get(llvm_int_type, data_layout_.getTypeAllocSize(allocated_llvm_type));
    if (allocation.size)
    {
        allocation.size->Accept(*this);
        auto array_size = result_;
        size_to_allocate = builder_.CreateMul(type_size, array_size);
    }
    else
    {
        size_to_allocate = type_size;
    }
    std::vector<llvm::Value*> arguments{size_to_allocate};

    allocation.initial_value->Accept(*this);
    auto initial_value = result_;

    auto reference = builder_.CreateCall(int_to_ptr, malloc_function.getCallee(), arguments, "allocation");

    // TODO use loop + GEP for array initialization
    builder_.CreateStore(initial_value, reference);
    result_ = reference;
    result_address_ = nullptr;
}

void Generator::GenerateFunctionBody(const Function& function, llvm::Function& llvm_function)
{
    llvm::BasicBlock* allocas_block = llvm::BasicBlock::Create(context_, kAllocationBlockName, &llvm_function);
    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(context_, "entry", &llvm_function);

    auto function_type = dynamic_pointer_cast<FunctionType>(function.type);

    builder_.SetInsertPoint(allocas_block);
    for (std::size_t i = 0; i < function.parameters->size(); ++i)
    {
        ParameterDeclaration& param = *function.parameters->at(i);
        llvm::Argument* llvm_param = llvm_function.args().begin() + i;

        auto param_type = type_converter_.Convert(*function_type->parameters->at(i));
        if (llvm::isa<llvm::FunctionType>(param_type))
        {
            param_type = llvm::PointerType::getUnqual(context_);
        }

        llvm::AllocaInst* alloca = builder_.CreateAlloca(param_type, nullptr, param.name);
        function.locals->SetLLVMValue(param.name, alloca);
        builder_.CreateStore(llvm_param, alloca);
    }

    builder_.SetInsertPoint(entry_block);
    function.body->Accept(*this);

    builder_.SetInsertPoint(allocas_block);
    builder_.CreateBr(entry_block);

    llvm::verifyFunction(llvm_function);
}

llvm::AllocaInst* Generator::GenerateAlloca(llvm::Type* type, std::string name)
{
    llvm::Function& llvm_function = *builder_.GetInsertBlock()->getParent();

    auto alloca_block = std::ranges::find(
        llvm_function, kAllocationBlockName, [](const llvm::BasicBlock& block) { return block.getName(); }
    );

    if (alloca_block == llvm_function.end())
    {
        throw GeneratorError(std::format(
            "Function '{}' does not have '{}' block. This should never happen.",
            llvm_function.getName().str(),
            kAllocationBlockName
        ));
    }

    auto previous_block = builder_.GetInsertBlock();
    builder_.SetInsertPoint(&*alloca_block);
    auto alloca = builder_.CreateAlloca(type, nullptr, name);
    builder_.SetInsertPoint(previous_block);

    return alloca;
}

void Generator::GenerateResultAddress()
{
    if (result_address_)
    {
        return;
    }

    result_address_ = GenerateAlloca(result_->getType(), std::format("address_{}", std::string{result_->getName()}));
    builder_.CreateStore(result_, result_address_);
}

std::vector<std::tuple<std::string, llvm::Value*>> Generator::GetActualMemberInitializers(
    const MemberInitializerList& explicit_initializers, const StructType& struct_type
)
{
    // clang-format off
    const auto explicitely_initialized_members = explicit_initializers
        | std::views::transform([](const auto& member_initializer) { return member_initializer->member; })
        | std::ranges::to<std::unordered_set>();

    auto default_initialized_members = *struct_type.members
        | std::views::transform([](const auto& member) { return member->name; })
        | std::views::filter([&](const std::string& member)
                             { return !explicitely_initialized_members.contains(member); });
    // clang-format on

    std::vector<std::tuple<std::string, llvm::Value*>> actual_initializers{};

    for (const std::string& member_name : default_initialized_members)
    {
        auto member = struct_type.GetMember(member_name);

        member->default_initializer->Accept(*this);
        auto init_value = result_;

        actual_initializers.push_back({member_name, init_value});
    }

    for (const auto& member_initializer : explicit_initializers)
    {
        member_initializer->value->Accept(*this);
        auto init_value = result_;

        actual_initializers.push_back({member_initializer->member, init_value});
    }

    return actual_initializers;
}

}  // namespace l0
