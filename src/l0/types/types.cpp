#include "l0/types/types.h"

#include <algorithm>
#include <sstream>

namespace l0
{

bool operator==(const Type& lhs, const Type& rhs) { return typeid(lhs) == typeid(rhs) && lhs.Equals(rhs); }

std::string ReferenceType::ToString() const { return std::format("&{}", base_type->ToString()); }

void ReferenceType::Accept(IConstTypeVisitor& visitor) const { visitor.Visit(*this); }

bool ReferenceType::Equals(const Type& other) const
{
    auto other_as_reference_type = dynamic_cast<const ReferenceType*>(&other);
    return other_as_reference_type && (*base_type == *other_as_reference_type->base_type);
}

std::string UnitType::ToString() const { return "()"; }

void UnitType::Accept(IConstTypeVisitor& visitor) const { visitor.Visit(*this); }

bool UnitType::Equals(const Type& other) const { return true; }

std::string BooleanType::ToString() const { return "Boolean"; }

void BooleanType::Accept(IConstTypeVisitor& visitor) const { visitor.Visit(*this); }

bool BooleanType::Equals(const Type& other) const { return true; }

std::string IntegerType::ToString() const { return "Integer"; }

void IntegerType::Accept(IConstTypeVisitor& visitor) const { visitor.Visit(*this); }

bool IntegerType::Equals(const Type& other) const { return true; }

std::string StringType::ToString() const { return "String"; }

void StringType::Accept(IConstTypeVisitor& visitor) const { visitor.Visit(*this); }

bool StringType::Equals(const Type& other) const { return true; }

std::string FunctionType::ToString() const
{
    std::stringstream ss{};
    ss << "(";
    for (const auto& parameter : *this->parameters)
    {
        ss << parameter->ToString() << ", ";
    }
    ss << ") -> " << this->return_type->ToString();
    return ss.str();
}

void FunctionType::Accept(IConstTypeVisitor& visitor) const { visitor.Visit(*this); }

bool FunctionType::Equals(const Type& other) const
{
    const FunctionType* real_other = static_cast<const FunctionType*>(&other);

    return *this->return_type == *real_other->return_type &&
           std::ranges::equal(
               *this->parameters, *real_other->parameters, [](const auto& lhs, const auto& rhs) { return *lhs == *rhs; }
           );
}

}  // namespace l0
