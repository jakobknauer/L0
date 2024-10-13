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

void GenerateIRForModule(l0::Module& module, llvm::LLVMContext& context, const fs::path& output_path);

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
        for (const auto& other_module : modules)
        {
            if (other_module->name != module->name)
            {
                module->externals->UpdateTypes(*other_module->globals);
            }
        }

        try
        {
            GlobalScopeBuilder{*module}.Run();
        }
        catch (const SemanticError& err)
        {
            std::println("Semantic error occured: {}", err.GetMessage());
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

    std::println("Generating IR");
    for (const auto& module : modules)
    {
        fs::path output_path{module->source_path};
        output_path.replace_extension("ll");
        GenerateIRForModule(*module, context, output_path);
    }

    std::println("Leaving.");
}

namespace l0
{

std::shared_ptr<l0::Module> GetModule(const fs::path& input_path)
{
    std::println("Loading source file '{}'", input_path.string());
    std::ifstream input_file{input_path};

    std::println("Lexical analysis");
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

    std::println("Syntactical analysis");
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

    std::println("Result:");
    AstPrinter{std::cout}.Print(*module);

    std::println("Analyzing top level statements");
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

    module.externals->DeclareType(Typename::Unit);
    module.externals->DeclareType(Typename::Boolean);
    module.externals->DeclareType(Typename::Integer);
    module.externals->DeclareType(Typename::Character);
    module.externals->DeclareType(Typename::CString);

    module.externals->DefineType(Typename::Unit, std::make_shared<UnitType>(TypeQualifier::Constant));
    module.externals->DefineType(Typename::Boolean, std::make_shared<BooleanType>(TypeQualifier::Constant));
    auto i64 = std::make_shared<IntegerType>(TypeQualifier::Constant);
    module.externals->DefineType(Typename::Integer, i64);
    auto c8 = std::make_shared<CharacterType>(TypeQualifier::Constant);
    module.externals->DefineType(Typename::Character, c8);
    auto cstring = std::make_shared<ReferenceType>(c8, TypeQualifier::Constant);
    module.externals->DefineType(Typename::CString, cstring);

    auto parameters_string = std::make_shared<ParameterList>(std::initializer_list<std::shared_ptr<Type>>{cstring});
    auto string_to_int = std::make_shared<FunctionType>(parameters_string, i64, TypeQualifier::Constant);
    module.externals->DeclareVariable("printf", string_to_int);

    auto void_to_char = std::make_shared<FunctionType>(std::make_shared<ParameterList>(), c8, TypeQualifier::Constant);
    module.externals->DeclareVariable("getchar", void_to_char);
}

void SemanticCheckModule(l0::Module& module)
{
    using namespace l0;

    std::println("Checking module '{}'", module.name);

    std::println("Resolving variables");
    try
    {
        Resolver{module}.Check();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("Checking types");
    try
    {
        Typechecker{module}.Check();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("Checking return statements");
    try
    {
        ReturnStatementPass{module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("Reference pass");
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

void GenerateIRForModule(l0::Module& module, llvm::LLVMContext& context, const fs::path& output_path)
{
    using namespace l0;

    std::println("Generating IR for module '{}'", module.name);
    try
    {
        std::string code = Generator{context, module}.Generate();
        std::ofstream output_file{output_path};
        output_file << code;
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
