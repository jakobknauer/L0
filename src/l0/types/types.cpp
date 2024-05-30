#include "l0/types/types.h"

#include <algorithm>
#include <sstream>

namespace l0
{

bool operator==(const Type& lhs, const Type& rhs) { return typeid(lhs) == typeid(rhs) && lhs.Equals(rhs); }

std::string BooleanType::ToString() const { return "Boolean"; }

bool BooleanType::Equals(const Type& other) const { return true; }

std::string IntegerType::ToString() const { return "Integer"; }

bool IntegerType::Equals(const Type& other) const { return true; }

std::string StringType::ToString() const { return "String"; }

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

bool FunctionType::Equals(const Type& other) const
{
    const FunctionType* real_other = static_cast<const FunctionType*>(&other);

    return *this->return_type == *real_other->return_type &&
           std::ranges::equal(
               *this->parameters, *real_other->parameters, [](const auto& lhs, const auto& rhs) { return *lhs == *rhs; }
           );
}

}  // namespace l0
