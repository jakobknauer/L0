#include "l0/types/types.h"

#include <algorithm>
#include <sstream>

namespace l0
{

std::string str(TypeQualifier qualifier)
{
    switch (qualifier)
    {
        case TypeQualifier::Constant:
            return "";
        case TypeQualifier::Mutable:
            return "mut ";
    }
}

Type::Type(TypeQualifier mutability)
    : mutability{mutability}
{
}

bool operator==(const Type& lhs, const Type& rhs)
{
    return typeid(lhs) == typeid(rhs) && lhs.Equals(rhs);
}

ReferenceType::ReferenceType(std::shared_ptr<Type> base_type, TypeQualifier mutability)
    : Type{mutability},
      base_type{base_type}
{
}

std::string ReferenceType::ToString() const
{
    return std::format("{}&{}", str(mutability), base_type->ToString());
}

void ReferenceType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool ReferenceType::Equals(const Type& other) const
{
    auto other_as_reference_type = dynamic_cast<const ReferenceType*>(&other);
    return other_as_reference_type && (*base_type == *other_as_reference_type->base_type);
}

UnitType::UnitType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string UnitType::ToString() const
{
    return str(mutability) + "()";
}

void UnitType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool UnitType::Equals(const Type& other) const
{
    return true;
}

BooleanType::BooleanType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string BooleanType::ToString() const
{
    return str(mutability) + "Boolean";
}

void BooleanType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool BooleanType::Equals(const Type& other) const
{
    return true;
}

IntegerType::IntegerType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string IntegerType::ToString() const
{
    return str(mutability) + "Integer";
}

void IntegerType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool IntegerType::Equals(const Type& other) const
{
    return true;
}

StringType::StringType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string StringType::ToString() const
{
    return str(mutability) + "String";
}

void StringType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool StringType::Equals(const Type& other) const
{
    return true;
}

FunctionType::FunctionType(
    std::shared_ptr<ParameterList> parameters, std::shared_ptr<Type> return_type, TypeQualifier mutability
)
    : Type{mutability},
      parameters{parameters},
      return_type{return_type}
{
}

std::string FunctionType::ToString() const
{
    std::stringstream ss{};
    ss << str(mutability);
    ss << "(";
    for (const auto& parameter : *this->parameters)
    {
        ss << parameter->ToString() << ", ";
    }
    ss << ") -> " << this->return_type->ToString();
    return ss.str();
}

void FunctionType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool FunctionType::Equals(const Type& other) const
{
    const FunctionType* real_other = static_cast<const FunctionType*>(&other);

    return (*this->return_type == *real_other->return_type)
        && std::ranges::equal(
               *this->parameters, *real_other->parameters, [](const auto& lhs, const auto& rhs) { return *lhs == *rhs; }
        );
}

StructType::StructType(std::string name, std::shared_ptr<StructMemberList> members, TypeQualifier mutability)
    : Type{mutability},
      name{name},
      members{members}
{
}

std::string StructType::ToString() const
{
    return name;
}

void StructType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool StructType::HasMember(std::string name) const
{
    return std::ranges::contains(*members, name, [](const auto& member) { return member->name; });
}

std::shared_ptr<StructMember> StructType::GetMember(std::string name) const
{
    return *std::ranges::find(*members, name, [](const auto& member) { return member->name; });
}

std::size_t StructType::GetMemberIndex(std::string name) const
{
    return std::ranges::find(*members, name, [](const auto& member) { return member->name; }) - members->begin();
}

bool StructType::Equals(const Type& other) const
{
    const StructType* real_other = static_cast<const StructType*>(&other);

    // TODO proper check?
    return this->name == real_other->name;
}

namespace
{

class ModifyQualifierVisitor : private IConstTypeVisitor
{
   public:
    std::shared_ptr<Type> ChangeQualifier(const Type& type, TypeQualifier qualifier)
    {
        qualifier_ = qualifier;
        type.Accept(*this);
        return result_;
    }

   private:
    TypeQualifier qualifier_;
    std::shared_ptr<Type> result_;

    void Visit(const ReferenceType& reference_type)
    {
        result_ = std::make_shared<ReferenceType>(reference_type.base_type, qualifier_);
    }

    void Visit(const UnitType& unit_type)
    {
        result_ = std::make_shared<UnitType>(qualifier_);
    }

    void Visit(const BooleanType& boolean_type)
    {
        result_ = std::make_shared<BooleanType>(qualifier_);
    }

    void Visit(const IntegerType& integer_type)
    {
        result_ = std::make_shared<IntegerType>(qualifier_);
    }

    void Visit(const StringType& string_type)
    {
        result_ = std::make_shared<StringType>(qualifier_);
    }

    void Visit(const FunctionType& function_type)
    {
        result_ = std::make_shared<FunctionType>(function_type.parameters, function_type.return_type, qualifier_);
    }

    void Visit(const StructType& struct_type)
    {
        result_ = std::make_shared<StructType>(struct_type.name, struct_type.members, qualifier_);
    }
};

}  // namespace

std::shared_ptr<Type> ModifyQualifier(const Type& type, TypeQualifier qualifier)
{
    return ModifyQualifierVisitor{}.ChangeQualifier(type, qualifier);
}

}  // namespace l0
