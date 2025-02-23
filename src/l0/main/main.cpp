#include <filesystem>
#include <print>

#include "l0/main/compiler_driver.h"

int main(int argc, char* argv[])
{
    using namespace l0;

    std::println("Hello, World!");
    CompilerDriver driver{};

    std::vector<std::filesystem::path> input_paths{argv + 1, argv + argc};

    driver.LoadModules(input_paths);
    driver.DeclareEnvironmentSymbols();
    driver.DeclareGlobalTypes();
    driver.DeclareExternalTypes();
    driver.DefineGlobalSymbols();
    driver.DeclareExternalVariables();
    driver.RunSemanticAnalysis();
    driver.GenerateIR();
    driver.StoreIR();

    std::println("Leaving");
}
