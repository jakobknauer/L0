#ifndef L0_SEMANTICS_TOP_LEVEL_ANALYZER
#define L0_SEMANTICS_TOP_LEVEL_ANALYZER

#include <memory>

#include "l0/ast/module.h"

namespace l0
{

class TopLevelAnalyzer
{
   public:
    TopLevelAnalyzer(Module& module);

    void Run();

   private:
    void DeclareType(std::shared_ptr<TypeDeclaration> type_declaration);

    Module& module_;
};

}  // namespace l0

#endif
