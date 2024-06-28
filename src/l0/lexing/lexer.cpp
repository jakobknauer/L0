#include "l0/lexing/lexer.h"

#include <format>
#include <ranges>

namespace l0
{

Lexer::Lexer(std::shared_ptr<std::istream> input)
    : input_{input}, keywords_{"return", "unit", "true", "false", "if", "else", "while"}
{
    single_character_operators_.insert(std::make_pair('(', TokenType::OpeningParen));
    single_character_operators_.insert(std::make_pair(')', TokenType::ClosingParen));
    single_character_operators_.insert(std::make_pair('{', TokenType::OpeningBrace));
    single_character_operators_.insert(std::make_pair('}', TokenType::ClosingBrace));
    single_character_operators_.insert(std::make_pair('+', TokenType::Plus));
    single_character_operators_.insert(std::make_pair('-', TokenType::Minus));
    single_character_operators_.insert(std::make_pair('*', TokenType::Asterisk));
    single_character_operators_.insert(std::make_pair('/', TokenType::Slash));
    single_character_operators_.insert(std::make_pair('!', TokenType::Bang));
    single_character_operators_.insert(std::make_pair(',', TokenType::Comma));
    single_character_operators_.insert(std::make_pair(':', TokenType::Colon));
    single_character_operators_.insert(std::make_pair(';', TokenType::Semicolon));
    single_character_operators_.insert(std::make_pair('=', TokenType::Equals));
    single_character_operators_.insert(std::make_pair('$', TokenType::Dollar));

    two_character_operators_.insert(std::make_pair("->", TokenType::Arrow));
    two_character_operators_.insert(std::make_pair("==", TokenType::EqualsEquals));
    two_character_operators_.insert(std::make_pair("!=", TokenType::BangEquals));
    two_character_operators_.insert(std::make_pair("&&", TokenType::AmpersandAmpersand));
    two_character_operators_.insert(std::make_pair("||", TokenType::PipePipe));

    for (auto c : single_character_operators_ | std::views::keys)
    {
        operator_characters_.insert(c);
    }
    for (auto s : two_character_operators_ | std::views::keys)
    {
        operator_characters_.insert(s[0]);
    }

    ReadAndSkip();
}

std::vector<Token> Lexer::GetTokens()
{
    std::vector<Token> tokens{};
    while (!AtEnd())
    {
        tokens.push_back(Next());
    }

    tokens.push_back(Token{
        .type = TokenType::EndOfFile,
        .lexeme = "EOF",
    });

    return tokens;
}

bool Lexer::AtEnd() const { return input_->eof(); }

char Lexer::Read() { return current_ = input_->get(); }

char Lexer::Skip()
{
    while (current_ == ' ' || current_ == '\n')
    {
        current_ = input_->get();
    }
    return current_;
}

char Lexer::ReadAndSkip()
{
    Read();
    return Skip();
}

Token Lexer::Next()
{
    if (operator_characters_.contains(current_))
    {
        char c1 = current_;
        char c2 = Read();
        std::string c1c2{c1, c2};

        TokenType type;
        std::string lexeme;

        if (two_character_operators_.contains(c1c2))
        {
            type = two_character_operators_.at(c1c2);
            lexeme = c1c2;
            ReadAndSkip();
        }
        else if (single_character_operators_.contains(c1))
        {
            type = single_character_operators_.at(c1);
            lexeme = c1;
            Skip();
        }
        else
        {
            throw LexerError(std::format("Cannot handle symbol '{}'.", c1));
        }

        Token token{
            .type = type,
            .lexeme = lexeme,
        };

        return token;
    }

    if (std::isalpha(current_))
    {
        return ReadIdentifierOrKeyword();
    }

    if (std::isdigit(current_))
    {
        return ReadIntegerLiteral();
    }

    if (current_ == '"')
    {
        return ReadStringLiteral();
    }

    throw LexerError(std::format("Unexpected character '{}'.", current_));
}

Token Lexer::ReadIdentifierOrKeyword()
{
    std::string lexeme{};
    while (std::isalpha(current_))
    {
        lexeme.append(std::string{current_});
        Read();
    }
    Skip();
    return Token{
        .type = keywords_.contains(lexeme) ? TokenType::Keyword : TokenType::Identifier,
        .lexeme = lexeme,
        .data = lexeme,
    };
}

Token Lexer::ReadIntegerLiteral()
{
    std::string number{};
    while (std::isdigit(current_))
    {
        number.append(std::string{current_});
        Read();
    }
    Skip();
    return Token{
        .type = TokenType::IntegerLiteral,
        .lexeme = number,
        .data = std::stol(number),
    };
}

Token Lexer::ReadStringLiteral()
{
    std::string string{};
    if (current_ != '"')
    {
        throw LexerError("String literal must begin with '\"'.");
    }
    Read();
    while (current_ != '"')
    {
        string += current_;
        Read();
    }
    Read();
    return Token{
        .type = TokenType::StringLiteral,
        .lexeme = std::format("\"{}\"", string),
        .data = string,
    };
}

LexerError::LexerError(std::string message) : message_{message} {}

std::string LexerError::GetMessage() const { return message_; }

}  // namespace l0
