#include <llvm/IR/LLVMContext.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>

#include "l0/ast/ast_printer.h"
#include "l0/ast/module.h"
#include "l0/common/constants.h"
#include "l0/generation/generator.h"
#include "l0/generation/generator_error.h"
#include "l0/lexing/lexer.h"
#include "l0/parsing/parser.h"
#include "l0/semantics/global_scope_builder.h"
#include "l0/semantics/reference_pass.h"
#include "l0/semantics/resolver.h"
#include "l0/semantics/return_statement_pass.h"
#include "l0/semantics/semantic_error.h"
#include "l0/semantics/top_level_analyzer.h"
#include "l0/semantics/typechecker.h"

namespace l0
{

namespace fs = std::filesystem;

void DeclareExternals(l0::Module& module);

std::shared_ptr<l0::Module> GetModule(const fs::path& input_path);

void SemanticCheckModule(l0::Module& module);

llvm::Module* GenerateIRForModule(l0::Module& module, llvm::LLVMContext& context);

}  // namespace l0

int main(int argc, char* argv[])
{
    using namespace l0;
    namespace fs = std::filesystem;

    std::println("Hello, World!");

    std::vector<fs::path> input_paths{argv + 1, argv + argc};

    std::println("Loading {} modules", input_paths.size());

    auto modules = input_paths | std::views::transform(GetModule) | std::ranges::to<std::vector>();

    std::println("Building global scope");
    for (const auto& module : modules)
    {
        std::println("\tFor module '{}'", module->name);

        std::println("\t\tUpdating externals");
        for (const auto& other_module : modules)
        {
            if (other_module->name != module->name)
            {
                module->externals->UpdateTypes(*other_module->globals);
            }
        }

        std::println("\t\tRun GlobalScopeBuilder");
        try
        {
            GlobalScopeBuilder{*module}.Run();
        }
        catch (const SemanticError& err)
        {
            std::println("Semantic error occured: {}", err.GetMessage());
            exit(-1);
        }
        catch (const ScopeError& err)
        {
            std::println("Scope error occured: {}", err.GetMessage());
            exit(-1);
        }
    }

    std::println("Semantic analysis");
    for (const auto& module : modules)
    {
        for (const auto& other_module : modules)
        {
            if (other_module->name != module->name)
            {
                module->externals->UpdateVariables(*other_module->globals);
            }
        }

        SemanticCheckModule(*module);
    }

    llvm::LLVMContext context{};
    auto pointer_type_ = llvm::PointerType::get(context, 0);
    auto closure_type_ = llvm::StructType::create(context, "__closure");
    closure_type_->setBody({pointer_type_, pointer_type_}, true);

    std::println("Generating IR");
    auto llvm_modules = modules
                      | std::views::transform([&](auto module) { return GenerateIRForModule(*module, context); })
                      | std::ranges::to<std::vector>();

    std::println("Saving IR to filesystem");
    for (const auto& [module, llvm_module] : std::views::zip(modules, llvm_modules))
    {
        fs::path output_path{module->source_path};
        output_path.replace_extension("ll");

        std::ofstream output_file{output_path};

        std::string code{};
        llvm::raw_string_ostream os{code};
        os << *llvm_module;

        output_file << code;
    }

    std::println("Leaving");
}

namespace l0
{

std::shared_ptr<l0::Module> GetModule(const fs::path& input_path)
{
    std::println("\tLoading source file '{}'", input_path.string());
    std::ifstream input_file{input_path};

    std::println("\t\tLexical analysis");
    std::vector<Token> tokens;
    try
    {
        tokens = Lexer{input_file}.GetTokens();
    }
    catch (const LexerError& le)
    {
        std::println("Lexer error occured: {}", le.GetMessage());
        exit(-1);
    }

    std::println("\t\tSyntactical analysis");
    std::shared_ptr<Module> module;
    try
    {
        module = Parser{tokens}.Parse();
    }
    catch (const ParserError& pe)
    {
        std::println("Parser error occured: {}", pe.GetMessage());
        exit(-1);
    }

    module->name = input_path.stem();
    module->source_path = input_path;

    DeclareExternals(*module);

//    std::println("Result:");
//    AstPrinter{std::cout}.Print(*module);

    std::println("\t\tAnalyzing top level statements");
    try
    {
        TopLevelAnalyzer{*module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    return module;
}

void DeclareExternals(l0::Module& module)
{
    using namespace l0;

    module.environment->DeclareType(Typename::Unit);
    module.environment->DeclareType(Typename::Boolean);
    module.environment->DeclareType(Typename::Integer);
    module.environment->DeclareType(Typename::Character);
    module.environment->DeclareType(Typename::CString);

    module.environment->DefineType(Typename::Unit, std::make_shared<UnitType>(TypeQualifier::Constant));
    module.environment->DefineType(Typename::Boolean, std::make_shared<BooleanType>(TypeQualifier::Constant));
    auto i64 = std::make_shared<IntegerType>(TypeQualifier::Constant);
    module.environment->DefineType(Typename::Integer, i64);
    auto c8 = std::make_shared<CharacterType>(TypeQualifier::Constant);
    module.environment->DefineType(Typename::Character, c8);
    auto cstring = std::make_shared<ReferenceType>(c8, TypeQualifier::Constant);
    module.environment->DefineType(Typename::CString, cstring);

    auto parameters_string = std::make_shared<ParameterList>(std::initializer_list<std::shared_ptr<Type>>{cstring});
    auto string_to_int = std::make_shared<FunctionType>(parameters_string, i64, TypeQualifier::Constant);
    module.environment->DeclareVariable("printf", string_to_int);

    auto void_to_char = std::make_shared<FunctionType>(std::make_shared<ParameterList>(), c8, TypeQualifier::Constant);
    module.environment->DeclareVariable("getchar", void_to_char);
}

void SemanticCheckModule(l0::Module& module)
{
    using namespace l0;

    std::println("\tFor module '{}'", module.name);

    std::println("\t\tResolving variables");
    try
    {
        Resolver{module}.Check();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("\t\tChecking types");
    try
    {
        Typechecker{module}.Check();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("\t\tChecking return statements");
    try
    {
        ReturnStatementPass{module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("\t\tReference pass");
    try
    {
        ReferencePass{module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }
}

llvm::Module* GenerateIRForModule(l0::Module& module, llvm::LLVMContext& context)
{
    using namespace l0;

    std::println("\tFor module '{}'", module.name);
    try
    {
        llvm::Module* llvm_module = Generator{context, module}.Generate();
        return llvm_module;
    }
    catch (const GeneratorError& ge)
    {
        std::println("Generator error occured: {}", ge.GetMessage());
        exit(-1);
    }
    catch (const ScopeError& se)
    {
        std::println("Scope error occured: {}", se.GetMessage());
        exit(-1);
    }
}

}  // namespace l0
