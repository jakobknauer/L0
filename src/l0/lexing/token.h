#ifndef L0_LEXING_TOKEN_H
#define L0_LEXING_TOKEN_H

#include <any>
#include <cstdint>
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
    EqualsEquals,
    BangEquals,
    AmpersandAmpersand,
    PipePipe,

    OpeningParen,
    ClosingParen,
    OpeningBrace,
    ClosingBrace,
    Comma,
    Colon,
    Semicolon,
    Equals,
    Arrow,
    Dollar,
};

std::string str(TokenType type);

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
