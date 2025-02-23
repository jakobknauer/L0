#include "l0/main/compiler_driver.h"

#include <fstream>
#include <print>

#include "l0/common/constants.h"
#include "l0/generation/generation.h"
#include "l0/generation/generator_error.h"
#include "l0/lexing/lexer.h"
#include "l0/lexing/token.h"
#include "l0/parsing/parser.h"
#include "l0/semantics/semantic_error.h"
#include "l0/semantics/semantics.h"

namespace l0
{

void CompilerDriver::LoadModules(const std::vector<std::filesystem::path>& paths)
{
    std::println("Loading {} module(s)", paths.size());
    for (const auto& path : paths)
    {
        LoadModule(path);
    }
}

void CompilerDriver::DeclareEnvironmentSymbols()
{
    std::println("Declaring environment symbols");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);
        FillEnvironmentScope(*module);
    }
}

void CompilerDriver::DeclareGlobalTypes()
{
    std::println("Declaring global types");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);
        try
        {
            RunTopLevelAnalysis(*module);
        }
        catch (const SemanticError& err)
        {
            std::println("Semantic error occured: {}", err.GetMessage());
            exit(-1);
        }
    }
}

void CompilerDriver::DeclareExternalTypes()
{
    std::println("Declaring external types");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);
        for (const auto& other_module : modules_)
        {
            if (other_module->name != module->name)
            {
                module->externals->UpdateTypes(*other_module->globals);
            }
        }
    }
}

void CompilerDriver::DefineGlobalSymbols()
{
    std::println("Defining global symbols");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);

        std::println("\t\tRun GlobalScopeBuilder");
        try
        {
            BuildGlobalScope(*module);
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
}

void CompilerDriver::DeclareExternalVariables()
{
    std::println("Declaring external variables");
    for (const auto& module : modules_)
    {
        for (const auto& other_module : modules_)
        {
            if (other_module->name != module->name)
            {
                module->externals->UpdateVariables(*other_module->globals);
            }
        }
    }
}

void CompilerDriver::RunSemanticAnalysis()
{
    std::println("Semantic analysis");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);
        SemanticCheckModule(*module);
    }
}

void CompilerDriver::GenerateIR()
{
    auto pointer_type_ = llvm::PointerType::get(context_, 0);
    auto closure_type_ = llvm::StructType::create(context_, "__closure");
    closure_type_->setBody({pointer_type_, pointer_type_}, true);

    std::println("Semantic analysis");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);
        GenerateIRForModule(*module);
    }
}

void CompilerDriver::StoreIR()
{
    std::println("Saving IR to filesystem");
    for (const auto& module : modules_)
    {
        std::println("\tFor module '{}'", module->name);
        StoreModuleIR(*module);
    }
}

void CompilerDriver::LoadModule(const std::filesystem::path& input_path)
{
    std::println("\tLoading source file '{}'", input_path.string());
    std::ifstream input_file{input_path};

    std::println("\t\tLexical analysis");
    std::vector<Token> tokens;
    try
    {
        tokens = Tokenize(input_file);
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
        module = Parse(tokens);
    }
    catch (const ParserError& pe)
    {
        std::println("Parser error occured: {}", pe.GetMessage());
        exit(-1);
    }

    module->name = input_path.stem();
    module->source_path = input_path;

    modules_.push_back(module);
}

void CompilerDriver::FillEnvironmentScope(Module& module)
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

void CompilerDriver::SemanticCheckModule(Module& module)
{
    std::println("\t\tResolving variables");
    try
    {
        BuildAndResolveLocalScopes(module);
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("\t\tChecking types");
    try
    {
        RunTypecheck(module);
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("\t\tChecking return statements");
    try
    {
        CheckReturnStatements(module);
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }

    std::println("\t\tReference pass");
    try
    {
        CheckReferences(module);
    }
    catch (const SemanticError& err)
    {
        std::println("Semantic error occured: {}", err.GetMessage());
        exit(-1);
    }
}

void CompilerDriver::GenerateIRForModule(Module& module)
{
    try
    {
        l0::GenerateIR(module, context_);
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

void CompilerDriver::StoreModuleIR(Module& module)
{
    std::filesystem::path output_path{module.source_path};
    output_path.replace_extension("ll");
    std::ofstream output_file{output_path};

    std::string textual_ir{};
    llvm::raw_string_ostream os{textual_ir};
    os << *module.intermediate_representation;

    output_file << textual_ir;
}

}  // namespace l0
