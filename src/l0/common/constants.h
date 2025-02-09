#ifndef L0_COMMON_CONSTANTS_H
#define L0_COMMON_CONSTANTS_H

#include <llvm-c/Error.h>

#include <string_view>

namespace l0::Keyword
{

constexpr std::string_view Constant{"const"};
constexpr std::string_view Delete{"delete"};
constexpr std::string_view Else{"else"};
constexpr std::string_view Enumeration{"enum"};
constexpr std::string_view False{"false"};
constexpr std::string_view Function{"fn"};
constexpr std::string_view If{"if"};
constexpr std::string_view Method{"method"};
constexpr std::string_view Mutable{"mut"};
constexpr std::string_view Namespace{"namespace"};
constexpr std::string_view New{"new"};
constexpr std::string_view Return{"return"};
constexpr std::string_view Structure{"struct"};
constexpr std::string_view True{"true"};
constexpr std::string_view Type{"type"};
constexpr std::string_view UnitLiteral{"unit"};
constexpr std::string_view While{"while"};

}  // namespace l0::Keyword

namespace l0::Typename
{

constexpr std::string_view Integer{"I64"};
constexpr std::string_view Unit{"()"};
constexpr std::string_view Boolean{"Boolean"};
constexpr std::string_view Character{"C8"};
constexpr std::string_view CString{"CString"};

}  // namespace l0::Typename

#endif  // L0_COMMON_CONSTANTS_H
