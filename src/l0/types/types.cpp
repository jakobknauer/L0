#include "l0/types/types.h"

#include <algorithm>
#include <format>
#include <ranges>
#include <sstream>

#include "l0/common/constants.h"

namespace l0
{

std::string str(TypeQualifier qualifier)
{
    switch (qualifier)
    {
        case TypeQualifier::Constant:
            return "";
        case TypeQualifier::Mutable:
            return std::format("{} ", Keyword::Mutable);
    }
    std::unreachable();
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

bool UnitType::Equals(const Type&) const
{
    return true;
}

BooleanType::BooleanType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string BooleanType::ToString() const
{
    return std::format("{}{}", str(mutability), Typename::Boolean);
}

void BooleanType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool BooleanType::Equals(const Type&) const
{
    return true;
}

IntegerType::IntegerType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string IntegerType::ToString() const
{
    return std::format("{}{}", str(mutability), Typename::Integer);
}

void IntegerType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool IntegerType::Equals(const Type&) const
{
    return true;
}

CharacterType::CharacterType(TypeQualifier mutability)
    : Type{mutability}
{
}

std::string CharacterType::ToString() const
{
    return std::format("{}{}", str(mutability), Typename::Character);
}

void CharacterType::Accept(IConstTypeVisitor& visitor) const
{
    visitor.Visit(*this);
}

bool CharacterType::Equals(const Type&) const
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
    return str(mutability) + name;
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

std::optional<std::size_t> StructType::GetNonstaticMemberIndex(std::string name) const
{
    // clang-format off
    auto nonstatic_members = *members
        | std::views::filter([](auto member) { return !member->is_static; })
        | std::views::enumerate;
    // clang-format on
    auto member_it = std::ranges::find(nonstatic_members, name, [](auto member) { return std::get<1>(member)->name; });

    if (member_it == nonstatic_members.end())
    {
        return {};
    }

    return std::get<0>(*member_it);
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

    void Visit(const UnitType&)
    {
        result_ = std::make_shared<UnitType>(qualifier_);
    }

    void Visit(const BooleanType&)
    {
        result_ = std::make_shared<BooleanType>(qualifier_);
    }

    void Visit(const IntegerType&)
    {
        result_ = std::make_shared<IntegerType>(qualifier_);
    }

    void Visit(const CharacterType&)
    {
        result_ = std::make_shared<CharacterType>(qualifier_);
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
