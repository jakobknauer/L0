#include "l0/ast/identifier.h"

#include <iostream>
#include <sstream>

namespace l0
{

Identifier::Identifier() {}

Identifier::Identifier(std::string_view only_part)
    : parts{std::string{only_part}}
{
}

Identifier::Identifier(std::string only_part)
    : parts{std::move(only_part)}
{
}

Identifier::Identifier(const char* only_part)
    : parts{only_part}
{
}

Identifier::Identifier(std::vector<std::string> parts)
    : parts{std::move(parts)}
{
}

std::string Identifier::last() const
{
    return parts.back();
}

std::string Identifier::ToString() const
{
    std::stringstream ss{};
    ss << *this;
    return ss.str();
}

bool Identifier::operator==(const Identifier& other) const
{
    return this->parts == other.parts;
}

Identifier& Identifier::operator+=(const Identifier& other)
{
    parts.insert(std::end(parts), std::begin(other.parts), std::end(other.parts));
    return *this;
}

Identifier Identifier::operator+(const Identifier& other) const
{
    Identifier result{*this};
    result += other;
    return result;
}

std::ostream& operator<<(std::ostream& stream, const Identifier& identifier)
{
    for (size_t i = 0; (i + 1) < identifier.parts.size(); ++i)
    {
        stream << identifier.parts[i] << "::";
    }
    if (identifier.parts.size() >= 1)
    {
        stream << identifier.parts.back();
    }
    return stream;
}

}  // namespace l0
