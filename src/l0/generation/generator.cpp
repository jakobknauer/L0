#include "l0/generation/generator.h"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/StripDeadPrototypes.h>

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
constexpr std::string kEntryBlockName{"entry"};

}  // namespace

Generator::Generator(llvm::LLVMContext& context, Module& module)
    : ast_module_{module},
      context_{context},
      builder_{context_},
      llvm_module_{new llvm::Module{module.name, context_}},
      type_converter_{context_}
{
    pointer_type_ = llvm::PointerType::get(context_, 0);
    closure_type_ = llvm::StructType::getTypeByName(context_, "__closure");
    int_type_ = llvm::IntegerType::getInt64Ty(context_);
}

llvm::Module* Generator::Generate()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    llvm_module_->setTargetTriple("x86_64-pc-linux-gnu");

    DeclareTypes();
    DeclareEnvironmentVariables();
    DeclareExternalVariables();
    DeclareGlobalVariables();
    DeclareCallables();

    DefineTypes();
    DefineGlobalVariables();
    DefineCallables();

    llvm::ModuleAnalysisManager mam;
    llvm::GlobalDCEPass global_dce_pass{};
    global_dce_pass.run(*llvm_module_, mam);

    return llvm_module_;
}

void Generator::DeclareTypes()
{
    for (const auto& type : ast_module_.externals->GetTypes())
    {
        llvm::StructType::create(context_, type);
    }
    for (const auto& type : ast_module_.globals->GetTypes())
    {
        llvm::StructType::create(context_, type);
    }
}

void Generator::DeclareEnvironmentVariables()
{
    for (const std::string& environment_symbol : ast_module_.environment->GetVariables())
    {
        auto type = ast_module_.environment->GetVariableType(environment_symbol);
        auto function_type = dynamic_pointer_cast<FunctionType>(type);
        auto llvm_type = type_converter_.Convert(*function_type);

        llvm::FunctionCallee function_callee = llvm_module_->getOrInsertFunction(environment_symbol, llvm_type);

        std::vector<llvm::Constant*> closure_members{};
        closure_members.push_back(llvm::dyn_cast<llvm::Function>(function_callee.getCallee()));
        closure_members.push_back(llvm::ConstantPointerNull::get(llvm::PointerType::get(context_, 0)));
        auto closure = llvm::ConstantStruct::get(closure_type_, closure_members);

        auto global_var = new llvm::GlobalVariable(
            *llvm_module_,
            closure_type_,
            true,
            llvm::GlobalValue::InternalLinkage,
            closure,
            std::format("{}", environment_symbol)
        );
        ast_module_.environment->SetLLVMValue(environment_symbol, global_var);
    }
}

void Generator::DeclareExternalVariables()
{
    for (const std::string& external_symbol : ast_module_.externals->GetVariables())
    {
        auto type = ast_module_.externals->GetVariableType(external_symbol);
        auto llvm_type = type_converter_.GetValueDeclarationType(*type);
        auto global_var = new llvm::GlobalVariable(
            *llvm_module_, llvm_type, true, llvm::GlobalValue::ExternalLinkage, nullptr, external_symbol
        );
        ast_module_.externals->SetLLVMValue(external_symbol, global_var);
    }
}

void Generator::DeclareGlobalVariables()
{
    for (const auto& global_symbol : ast_module_.globals->GetVariables())
    {
        if (global_symbol == "main")
        {
            continue;
        }
        const auto type = ast_module_.globals->GetVariableType(global_symbol);
        const auto llvm_type = type_converter_.GetValueDeclarationType(*type);
        const auto global_var = new llvm::GlobalVariable(
            *llvm_module_, llvm_type, true, llvm::GlobalValue::ExternalLinkage, nullptr, global_symbol
        );
        ast_module_.globals->SetLLVMValue(global_symbol, global_var);
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

    const auto llvm_type = type_converter_.GetFunctionDeclarationType(*type);
    const auto linkage = (function->global_name == "main") ? llvm::GlobalValue::LinkageTypes::ExternalLinkage
                                                           : llvm::GlobalValue::LinkageTypes::PrivateLinkage;
    llvm::Function::Create(llvm_type, linkage, function->global_name.value(), llvm_module_);
}

void Generator::DefineTypes()
{
    for (const auto& type_declaration : ast_module_.global_type_declarations)
    {
        auto struct_type = dynamic_pointer_cast<StructType>(type_declaration->type);
        if (!struct_type)
        {
            continue;
        }
        auto llvm_struct_type = llvm::StructType::getTypeByName(context_, struct_type->name);

        // clang-format off
        auto non_static_members = *struct_type->members
            | std::views::filter([](auto member) { return !member->is_static; })
            | std::views::transform([&](auto member) { return type_converter_.GetValueDeclarationType(*member->type); })
            | std::ranges::to<std::vector>();
        // clang-format on

        llvm_struct_type->setBody(non_static_members, true);

        for (auto member : *struct_type->members)
        {
            if (!member->default_initializer)
            {
                continue;
            }
            member->default_initializer->Accept(*this);
            const auto initializer = llvm::dyn_cast<llvm::Constant>(result_);
            const auto llvm_value = ast_module_.globals->GetLLVMValue(member->default_initializer_global_name.value());
            const auto global_var = llvm::dyn_cast<llvm::GlobalVariable>(llvm_value);
            global_var->setInitializer(initializer);
        }
    }
}

void Generator::DefineGlobalVariables()
{
    for (const auto& global_declaration : ast_module_.global_declarations)
    {
        if (global_declaration->variable == "main")
        {
            continue;
        }

        global_declaration->initializer->Accept(*this);
        auto initializer = llvm::dyn_cast<llvm::Constant>(result_);

        const auto llvm_value = ast_module_.globals->GetLLVMValue(global_declaration->variable);
        const auto global_var = llvm::dyn_cast<llvm::GlobalVariable>(llvm_value);

        global_var->setInitializer(initializer);
    }
}

void Generator::DefineCallables()
{
    for (auto callable : ast_module_.callables)
    {
        llvm::Function* llvm_function = llvm_module_->getFunction(callable->global_name.value());
        if (!llvm_function)
        {
            throw GeneratorError(
                std::format("Callable with name '{}' has not been declared.", callable->global_name.value())
            );
        }
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

    llvm::FunctionCallee free_function = llvm_module_->getOrInsertFunction("free", ptr_to_void);

    deallocation.reference->Accept(*this);
    llvm::Value* operand = result_;
    std::vector<llvm::Value*> arguments{operand};

    result_ = builder_.CreateCall(ptr_to_void, free_function.getCallee(), arguments);
}

void Generator::Visit(const Assignment& assignment)
{
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
            result_ = builder_.CreateLoad(llvm_type, address, std::format("dereftmp_{}", result_->getName().str()));
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
        VisitGlobal(global_variable);
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
        auto llvm_type = type_converter_.GetValueDeclarationType(*variable.type);
        result_ = builder_.CreateLoad(llvm_type, llvm_value, variable.name);
        result_address_ = llvm_value;
    }
}

void Generator::Visit(const MemberAccessor& member_accessor)
{
    auto struct_name = member_accessor.object_type->name;

    member_accessor.object->Accept(*this);
    GenerateResultAddress();
    auto object_ptr = result_address_;
    auto llvm_struct_type = type_converter_.Convert(*member_accessor.object_type);
    auto llvm_member_type = type_converter_.GetValueDeclarationType(*member_accessor.type);

    if (member_accessor.nonstatic_member_index)
    {
        auto member_address = builder_.CreateConstGEP2_32(
            llvm_struct_type,
            object_ptr,
            0,
            member_accessor.nonstatic_member_index.value(),
            std::format("geptmp_{}.{}::{}", object_ptr->getName().str(), struct_name, member_accessor.member)
        );

        result_ = builder_.CreateLoad(
            llvm_member_type,
            member_address,
            std::format("{}.{}::{}", object_ptr->getName().str(), struct_name, member_accessor.member)
        );
        result_address_ = member_address;
    }
    else
    {
        auto member = member_accessor.object_type->GetMember(member_accessor.member);
        auto static_initializer =
            member_accessor.object_type_scope->GetLLVMValue(*member->default_initializer_global_name);
        auto static_initializer_as_global = llvm::dyn_cast<llvm::GlobalVariable>(static_initializer);
        if (!static_initializer_as_global)
        {
            throw GeneratorError(std::format("Static initializer for {} is not a global variable.", member->name));
        }
        VisitGlobal(static_initializer_as_global);
        // leave result_ and result_address_ as is
    }

    object_ptr_ = object_ptr;
}

void Generator::Visit(const Call& call)
{
    call.function->Accept(*this);

    GenerateResultAddress();
    llvm::Value* closure = result_address_;
    llvm::Value* object_ptr = object_ptr_;

    const std::string& closure_name = object_ptr
                                        ? std::format("{}.{}", object_ptr_->getName().str(), closure->getName().str())
                                        : closure->getName().str();

    object_ptr_ = nullptr;

    auto function_address =
        builder_.CreateConstGEP2_32(closure_type_, closure, 0, 0, std::format("geptmp_{}_function", closure_name));
    auto context_address =
        builder_.CreateConstGEP2_32(closure_type_, closure, 0, 1, std::format("geptmp_{}_context", closure_name));
    auto llvm_function = builder_.CreateLoad(
        llvm::PointerType::get(context_, 0), function_address, std::format("{}_function", closure_name)
    );
    auto context = builder_.CreateLoad(
        llvm::PointerType::get(context_, 0), context_address, std::format("{}_context", closure_name)
    );

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
    arguments.push_back(context);

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
    result_ = builder_.CreateGlobalString(literal.value, "__const_str", 0, llvm_module_);
    result_address_ = nullptr;
}

void Generator::Visit(const Function& function)
{
    if (!function.global_name)
    {
        function.global_name = GetLambdaName();
    }

    llvm::Function* closure_function = llvm_module_->getFunction(function.global_name.value());
    llvm::Value* closure_context_ptr = nullptr;

    if (!closure_function)
    {
        llvm::StructType* context_struct = nullptr;
        if (function.captures)
        {
            std::tie(closure_context_ptr, context_struct) = GenerateClosureContext(function);
        }

        auto type = dynamic_pointer_cast<FunctionType>(function.type);
        auto llvm_type = type_converter_.GetFunctionDeclarationType(*type);
        auto linkage = llvm::GlobalValue::LinkageTypes::ExternalLinkage;
        closure_function = llvm::Function::Create(llvm_type, linkage, function.global_name.value(), llvm_module_);

        GenerateFunctionBody(function, *closure_function, context_struct);
    }

    if (!function.captures)
    {
        std::vector<llvm::Constant*> closure_members{};
        closure_members.push_back(closure_function);
        closure_members.push_back(llvm::ConstantPointerNull::get(pointer_type_));
        auto closure = llvm::ConstantStruct::get(closure_type_, closure_members);

        result_ = closure;
        result_address_ = nullptr;
    }
    else
    {
        auto closure_address = GenerateAlloca(closure_type_, std::format("address_{}", function.global_name.value()));

        auto closure_function_address = builder_.CreateConstGEP2_32(
            closure_type_, closure_address, 0, 0, std::format("geptmp_{}_function", function.global_name.value())
        );
        auto closure_context_address = builder_.CreateConstGEP2_32(
            closure_type_, closure_address, 0, 1, std::format("geptmp_{}_captures", function.global_name.value())
        );
        builder_.CreateStore(closure_function, closure_function_address);
        builder_.CreateStore(closure_context_ptr, closure_context_address);

        result_ = builder_.CreateLoad(closure_type_, closure_address);
        result_address_ = nullptr;
    }
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

    const std::string object_name = std::format("init_{}", struct_type->name);

    llvm::Type* llvm_type = type_converter_.Convert(*initializer.type);
    auto alloca = GenerateAlloca(llvm_type, std::format("address_{}", object_name));

    auto actual_initializers =
        GetActualMemberInitializers(*initializer.member_initializers, *struct_type, *initializer.type_scope);

    for (const auto& [name, init_value] : actual_initializers)
    {
        auto member_index = struct_type->GetNonstaticMemberIndex(name);
        auto member_address = builder_.CreateConstGEP2_32(
            llvm_type,
            alloca,
            0,
            member_index.value(),
            std::format("address_{}.{}::{}", object_name, struct_type->name, name)
        );
        builder_.CreateStore(init_value, member_address);
    }

    result_ = builder_.CreateLoad(llvm_type, alloca, object_name);
    result_address_ = alloca;
}

void Generator::Visit(const Allocation& allocation)
{
    llvm::Type* allocated_llvm_type = type_converter_.Convert(*allocation.allocated_type);
    llvm::Value* type_size = llvm::ConstantInt::get(int_type_, data_layout_.getTypeAllocSize(allocated_llvm_type));

    llvm::Value* size_to_allocate;
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
    allocation.initial_value->Accept(*this);
    auto initial_value = result_;

    auto reference = CallMalloc(size_to_allocate, "allocated");

    // TODO use loop + GEP for array initialization
    builder_.CreateStore(initial_value, reference);
    result_ = reference;
    result_address_ = nullptr;
}

void Generator::GenerateFunctionBody(
    const Function& function, llvm::Function& llvm_function, llvm::StructType* context_struct
)
{
    llvm::BasicBlock* previous_block = builder_.GetInsertBlock();

    llvm::BasicBlock* allocas_block = llvm::BasicBlock::Create(context_, kAllocationBlockName, &llvm_function);
    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(context_, kEntryBlockName, &llvm_function);

    auto function_type = dynamic_pointer_cast<FunctionType>(function.type);

    builder_.SetInsertPoint(allocas_block);
    // Fill local scope with captures
    if (function.captures)
    {
        auto context_address = (llvm_function.args().end() - 1);
        for (auto capture_index : std::views::iota(std::size_t{0}, function.captures->size()))
        {
            const Variable& capture = *function.captures->at(capture_index);
            const auto& capture_address =
                builder_.CreateConstGEP2_32(context_struct, context_address, 0, capture_index, capture.name);
            function.locals->SetLLVMValue(capture.name, capture_address);
        }
    }

    // Fill local scope with arguments
    for (std::size_t i = 0; i < function.parameters->size(); ++i)
    {
        ParameterDeclaration& param = *function.parameters->at(i);
        llvm::Argument* llvm_param = llvm_function.args().begin() + i;

        auto param_type = type_converter_.GetValueDeclarationType(*function_type->parameters->at(i));

        llvm::AllocaInst* alloca = builder_.CreateAlloca(param_type, nullptr, param.name);
        function.locals->SetLLVMValue(param.name, alloca);
        builder_.CreateStore(llvm_param, alloca);
    }

    builder_.SetInsertPoint(entry_block);
    function.body->Accept(*this);

    builder_.SetInsertPoint(allocas_block);
    builder_.CreateBr(entry_block);

    llvm::verifyFunction(llvm_function);

    builder_.SetInsertPoint(previous_block);
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

    result_address_ = GenerateAlloca(result_->getType(), std::format("address_{}", result_->getName().str()));
    builder_.CreateStore(result_, result_address_);
}

std::vector<std::tuple<std::string, llvm::Value*>> Generator::GetActualMemberInitializers(
    const MemberInitializerList& explicit_initializers, const StructType& struct_type, const Scope& scope
)
{
    // clang-format off
    const auto explicitely_initialized_members = explicit_initializers
        | std::views::transform([](const auto& member_initializer) { return member_initializer->member; })
        | std::ranges::to<std::unordered_set>();

    auto default_initialized_members = *struct_type.members
        | std::views::filter([](auto member) { return !member->is_static;})
        | std::views::transform([](auto member) { return member->name; })
        | std::views::filter([&](const std::string& member)
                             { return !explicitely_initialized_members.contains(member); });
    // clang-format on

    std::vector<std::tuple<std::string, llvm::Value*>> actual_initializers{};

    for (const std::string& member_name : default_initialized_members)
    {
        auto member = struct_type.GetMember(member_name);
        auto default_initializer = scope.GetLLVMValue(*member->default_initializer_global_name);
        auto default_initializer_as_global = llvm::dyn_cast<llvm::GlobalVariable>(default_initializer);
        if (!default_initializer_as_global)
        {
            throw GeneratorError(std::format("Default initializer for {} is not a global variable.", member->name));
        }
        VisitGlobal(default_initializer_as_global);

        actual_initializers.push_back({member_name, result_});
        result_ = nullptr;
    }

    for (const auto& member_initializer : explicit_initializers)
    {
        member_initializer->value->Accept(*this);
        auto init_value = result_;

        actual_initializers.push_back({member_initializer->member, init_value});
    }

    return actual_initializers;
}

llvm::StructType* Generator::GenerateClosureContextStruct(const Function& function)
{
    std::vector<llvm::Type*> capture_types{};

    for (const auto& capture : *function.captures)
    {
        auto type = capture->type;
        auto llvm_type = type_converter_.GetValueDeclarationType(*type);
        capture_types.push_back(llvm_type);
    }

    auto closure_context_struct =
        llvm::StructType::create(context_, std::format("__context__{}", *function.global_name));
    closure_context_struct->setBody(capture_types, true);

    return closure_context_struct;
}

void Generator::VisitGlobal(llvm::GlobalVariable* global_variable)
{
    auto llvm_type = global_variable->getValueType();
    if (llvm::isa<llvm::ArrayType>(llvm_type))
    {
        result_ = global_variable;
        result_address_ = nullptr;
    }
    else
    {
        result_ = builder_.CreateLoad(llvm_type, global_variable, global_variable->getName());
        result_address_ = global_variable;
    }
}

llvm::Value* Generator::CallMalloc(llvm::Value* size, const std::string& name)
{
    llvm::FunctionType* int_to_ptr = llvm::FunctionType::get(pointer_type_, int_type_, false);
    llvm::Function* malloc_function =
        llvm::dyn_cast<llvm::Function>(llvm_module_->getOrInsertFunction("malloc", int_to_ptr).getCallee());
    return builder_.CreateCall(int_to_ptr, malloc_function, {size}, name);
}

std::tuple<llvm::Value*, llvm::StructType*> Generator::GenerateClosureContext(const Function& function)
{
    auto context_struct = GenerateClosureContextStruct(function);

    llvm::Value* context_size = llvm::ConstantInt::get(int_type_, data_layout_.getTypeAllocSize(context_struct));
    auto context_address = CallMalloc(context_size, std::format("address_{}_context", function.global_name.value()));

    for (auto capture_index : std::views::iota(std::size_t{0}, function.captures->size()))
    {
        const auto& capture = function.captures->at(capture_index);
        capture->Accept(*this);
        const auto captured_value = result_;

        auto member_address = builder_.CreateConstGEP2_32(
            context_struct,
            context_address,
            0,
            capture_index,
            std::format("tmpgep_{}_capture_{}", function.global_name.value(), capture->name)
        );
        builder_.CreateStore(captured_value, member_address);
    }

    return {context_address, context_struct};
}

}  // namespace l0
