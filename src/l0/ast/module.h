#ifndef L0_AST_MODULE_H
#define L0_AST_MODULE_H

#include <memory>

#include "l0/ast/scope.h"
#include "l0/ast/statement.h"

namespace l0
{

class Module
{
   public:
    std::unique_ptr<StatementBlock> statements{};
    std::string name{};
    std::shared_ptr<Scope> globals = std::make_shared<Scope>();
    std::shared_ptr<Scope> externals = std::make_shared<Scope>();
};

}  // namespace l0

#endif
