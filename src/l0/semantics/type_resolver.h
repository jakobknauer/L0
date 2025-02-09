#ifndef L0_SEMANTICS_TYPE_RESOLVER_H
#define L0_SEMANTICS_TYPE_RESOLVER_H

#include <memory>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/type_annotation.h"
#include "l0/types/types.h"

namespace l0::detail
{

class TypeResolver : private ITypeAnnotationVisitor
{
   public:
    TypeResolver(const Module& module);

    std::shared_ptr<Type> Convert(const TypeAnnotation& annotation, Identifier namespace_);
    TypeQualifier Convert(TypeAnnotationQualifier qualifier);
    std::shared_ptr<FunctionType> Convert(const Function& function, Identifier namespace_);
    std::shared_ptr<Type> GetTypeByName(const Identifier& identifier, Identifier namespace_);
    std::pair<std::shared_ptr<Scope>, Identifier> Resolve(const Identifier& identifier, Identifier namespace_);

   private:
    const Module& module_;
    std::shared_ptr<Type> result_;
    Identifier namespace_;

    std::optional<std::shared_ptr<Scope>> Resolve(const Identifier& identifier);

    void Visit(const SimpleTypeAnnotation& sta) override;
    void Visit(const ReferenceTypeAnnotation& rta) override;
    void Visit(const FunctionTypeAnnotation& fta) override;
    void Visit(const MethodTypeAnnotation& mta) override;
    void Visit(const MutabilityOnlyTypeAnnotation& mota) override;
};

}  // namespace l0::detail

#endif
