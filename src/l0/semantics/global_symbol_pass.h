#ifndef L0_SEMANTICS_GLOBAL_SYMBOL_PASS_H
#define L0_SEMANTICS_GLOBAL_SYMBOL_PASS_H

#include "l0/ast/module.h"
#include "l0/semantics/type_annotation_converter.h"

namespace l0
{

class GlobalSymbolPass
{
   public:
    GlobalSymbolPass(Module& module);

    void Run();

   private:
    Module& module_;
    TypeAnnotationConverter converter_{};
};

}  // namespace l0

#endif
