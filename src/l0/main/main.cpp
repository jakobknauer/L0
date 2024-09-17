#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>

#include "l0/ast/ast_printer.h"
#include "l0/generation/generator.h"
#include "l0/generation/generator_error.h"
#include "l0/lexing/lexer.h"
#include "l0/parsing/parser.h"
#include "l0/semantics/global_scope_builder.h"
#include "l0/semantics/reference_pass.h"
#include "l0/semantics/resolver.h"
#include "l0/semantics/return_statement_pass.h"
#include "l0/semantics/semantic_error.h"
#include "l0/semantics/typechecker.h"

int main(int argc, char* argv[])
{
    using namespace ::l0;
    namespace fs = ::std::filesystem;

    std::println("Hello, World!");

    fs::path input_path{argv[1]};

    auto input_file = std::make_shared<std::ifstream>(input_path);

    std::println("Lexical analysis...");
    std::vector<Token> tokens;
    try
    {
        tokens = Lexer{input_file}.GetTokens();
    }
    catch (const LexerError& le)
    {
        std::println("Lexer error occured: {}", le.GetMessage());
        return -1;
    }

    std::println("Syntactical analysis...");
    std::shared_ptr<Module> module;
    try
    {
        module = Parser{tokens}.Parse();
    }
    catch (const ParserError& pe)
    {
        std::println("Parser error occured: {}", pe.GetMessage());
        return -1;
    }

    std::println("Result:");
    AstPrinter{std::cout}.Print(*module);

    module->name = "MainModule";

    auto character_type = std::make_shared<CharacterType>(TypeQualifier::Constant);
    auto parameters = std::make_shared<ParameterList>(std::initializer_list<std::shared_ptr<Type>>{
        std::make_shared<ReferenceType>(character_type, TypeQualifier::Constant)
    });
    auto integer_type = std::make_shared<IntegerType>(TypeQualifier::Constant);
    auto string_to_int = std::make_shared<FunctionType>(parameters, integer_type, TypeQualifier::Constant);
    auto void_to_char = std::make_shared<FunctionType>(std::make_shared<ParameterList>(), character_type, TypeQualifier::Constant);

    module->externals->DeclareVariable("printf", string_to_int);
    module->externals->DeclareVariable("getchar", void_to_char);

    std::println("Semantical analysis...");

    std::println("Building global scope...");
    try
    {
        GlobalScopeBuilder{*module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        return -1;
    }

    std::println("Resolving variables...");
    try
    {
        Resolver{*module}.Check();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        return -1;
    }

    std::println("Checking types...");
    try
    {
        Typechecker{*module}.Check();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        return -1;
    }

    std::println("Checking return statements...");
    try
    {
        ReturnStatementPass{*module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        return -1;
    }

    std::println("Reference pass...");
    try
    {
        ReferencePass{*module}.Run();
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        return -1;
    }

    std::println("Compiling...");
    try
    {
        std::string code = Generator{*module}.Generate();
        fs::path output_path = input_path;
        output_path.replace_extension("ll");
        std::ofstream output_file{output_path};
        output_file << code;
    }
    catch (const GeneratorError& ge)
    {
        std::println("Generator error occured: {}", ge.GetMessage());
        return -1;
    }
    catch (const ScopeError& se)
    {
        std::println("Scope error occured: {}", se.GetMessage());
        return -1;
    }

    std::println("Leaving.");
}
