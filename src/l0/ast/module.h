#ifndef L0_AST_MODULE_H
#define L0_AST_MODULE_H

#include <filesystem>
#include <memory>

#include "l0/ast/expression.h"
#include "l0/ast/scope.h"
#include "l0/ast/statement.h"

namespace l0
{

class Module
{
   public:
    std::shared_ptr<StatementBlock> statements{};

    std::string name;
    std::filesystem::path source_path;

    std::shared_ptr<Scope> globals = std::make_shared<Scope>();
    std::shared_ptr<Scope> externals = std::make_shared<Scope>();

    std::vector<std::shared_ptr<Function>> callables{};
    std::vector<std::shared_ptr<Declaration>> global_declarations{};
    std::vector<std::shared_ptr<TypeDeclaration>> global_type_declarations{};
};

}  // namespace l0

#endif
