#ifndef L0_TYPES_TYPES_H
#define L0_TYPES_TYPES_H

#include <llvm/IR/Attributes.h>

#include <memory>
#include <string>
#include <vector>

namespace l0
{

class IConstTypeVisitor;

enum class TypeQualifier
{
    Constant,
    Mutable,
};

class Type
{
   public:
    Type() = delete;
    Type(TypeQualifier mutability);
    virtual ~Type() = default;

    virtual std::string ToString() const = 0;

    virtual void Accept(IConstTypeVisitor& visitor) const = 0;

    const TypeQualifier mutability;

   protected:
    friend bool operator==(const Type& lhs, const Type& rhs);
    virtual bool Equals(const Type& other) const = 0;
};

class ReferenceType : public Type
{
   public:
    ReferenceType(std::shared_ptr<Type> base_type, TypeQualifier mutability);

    std::string ToString() const override;

    const std::shared_ptr<Type> base_type;

    void Accept(IConstTypeVisitor& visitor) const override;

   protected:
    bool Equals(const Type& other) const override;
};

class UnitType : public Type
{
   public:
    UnitType(TypeQualifier mutability);

    std::string ToString() const override;

    void Accept(IConstTypeVisitor& visitor) const override;

   protected:
    bool Equals(const Type& other) const override;
};

class BooleanType : public Type
{
   public:
    BooleanType(TypeQualifier mutability);

    std::string ToString() const override;

    void Accept(IConstTypeVisitor& visitor) const override;

   protected:
    bool Equals(const Type& other) const override;
};

class IntegerType : public Type
{
   public:
    IntegerType(TypeQualifier mutability);

    std::string ToString() const override;

    void Accept(IConstTypeVisitor& visitor) const override;

   protected:
    bool Equals(const Type& other) const override;
};

class StringType : public Type
{
   public:
    StringType(TypeQualifier mutability);

    std::string ToString() const override;

    void Accept(IConstTypeVisitor& visitor) const override;

   protected:
    bool Equals(const Type& other) const override;
};

using ParameterList = const std::vector<std::shared_ptr<Type>>;

class FunctionType : public Type
{
   public:
    FunctionType(
        std::shared_ptr<ParameterList> parameters, std::shared_ptr<Type> return_type, TypeQualifier mutability
    );

    std::string ToString() const override;

    void Accept(IConstTypeVisitor& visitor) const override;

    const std::shared_ptr<ParameterList> parameters = std::make_unique<ParameterList>();
    const std::shared_ptr<Type> return_type;

   protected:
    bool Equals(const Type& other) const override;
};

class IConstTypeVisitor
{
   public:
    ~IConstTypeVisitor() = default;

    virtual void Visit(const ReferenceType& reference_type) = 0;
    virtual void Visit(const UnitType& unit_type) = 0;
    virtual void Visit(const BooleanType& boolean_type) = 0;
    virtual void Visit(const IntegerType& integer_type) = 0;
    virtual void Visit(const StringType& string_type) = 0;
    virtual void Visit(const FunctionType& function_type) = 0;
};

}  // namespace l0

#endif
