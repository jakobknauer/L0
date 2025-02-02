#include "l0/ast/identifier.h"

#include <iostream>

namespace l0
{

Identifier::Identifier(std::string_view only_part)
    : parts{std::string{only_part}}
{
}

Identifier::Identifier(std::vector<std::string> parts)
    : parts{parts}
{
}

std::string Identifier::last() const
{
    return parts.back();
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
