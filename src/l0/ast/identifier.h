#ifndef L0_AST_IDENTIFIER
#define L0_AST_IDENTIFIER

#include <string>
#include <vector>

namespace l0
{

struct Identifier
{
    Identifier(std::string_view only_part);
    Identifier(std::vector<std::string> parts);

    std::vector<std::string> parts{};

    std::string last() const;
};

std::ostream& operator<<(std::ostream& stream, const Identifier& identifier);

}  // namespace l0

#endif
