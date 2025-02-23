#ifndef L0_SEMANTICS_FILL_TYPES
#define L0_SEMANTICS_FILL_TYPES

#include <memory>

#include "l0/ast/module.h"
#include "l0/semantics/type_resolver.h"

namespace l0::detail
{

void FillTypeDetails(Module& module);

void FillStructDetails(
    Module& module, std::shared_ptr<StructType> type, const StructExpression& definition, TypeResolver& type_resolver
);

void FillEnumDetails(Module& module, std::shared_ptr<EnumType> type, const EnumExpression& definition);

}  // namespace l0::detail

#endif
