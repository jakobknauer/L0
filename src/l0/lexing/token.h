#ifndef L0_LEXING_TOKEN_H
#define L0_LEXING_TOKEN_H

#include <any>
#include <cstdint>
#include <sstream>
#include <string>

namespace l0
{

enum class TokenType
{
    None,
    EndOfFile,

    Keyword,
    Identifier,

    IntegerLiteral,
    StringLiteral,

    Plus,
    Minus,
    Asterisk,
    Slash,
    Bang,
    Ampersand,
    Caret,
    EqualsEquals,
    BangEquals,
    AmpersandAmpersand,
    PipePipe,

    OpeningParen,
    ClosingParen,
    OpeningBracket,
    ClosingBracket,
    OpeningBrace,
    ClosingBrace,
    Dot,
    Comma,
    Colon,
    Semicolon,
    Equals,
    Arrow,
    Dollar,
    ColonEquals,
};

std::string str(TokenType type);

std::string str(const auto& types)
{
    std::stringstream ss{};
    ss << "{";
    for (auto type : types)
    {
        ss << str(type) << ",";
    }
    ss << "}";
    return ss.str();
}

struct Token
{
    using LineType = std::int32_t;
    using DataType = std::any;

    TokenType type{};
    LineType line{};
    std::string lexeme{};
    DataType data{};
};

}  // namespace l0

#endif
