#ifndef L0_AST_IDENTIFIER
#define L0_AST_IDENTIFIER

#include <string>
#include <vector>

namespace l0
{

class Identifier
{
   public:
    Identifier();
    Identifier(std::string_view only_part);
    Identifier(std::string only_part);
    Identifier(const char* only_part);
    Identifier(std::vector<std::string> parts);

    std::string ToString() const;
    Identifier GetPrefix() const;

    bool operator==(const Identifier& other) const;
    Identifier& operator+=(const Identifier& other);
    Identifier operator+(const Identifier& other) const;

    friend std::ostream& operator<<(std::ostream& stream, const Identifier& identifier);

   private:
    std::string last() const;

    std::vector<std::string> parts{};
};

std::ostream& operator<<(std::ostream& stream, const Identifier& identifier);

}  // namespace l0

template <>
struct std::hash<l0::Identifier>
{
    std::size_t operator()(const l0::Identifier& identifier) const
    {
        return std::hash<std::string>()(identifier.ToString());
    }
};

#endif
