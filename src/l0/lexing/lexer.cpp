#include "l0/lexing/lexer.h"

#include <format>
#include <ranges>

namespace l0
{

Lexer::Lexer(std::shared_ptr<std::istream> input)
    : input_{input}, keywords_{"return", "unit", "true", "false", "if", "else", "while", "new"}
{
    single_character_operators_ = {
        {'(', TokenType::OpeningParen},
        {')', TokenType::ClosingParen},
        {'{', TokenType::OpeningBrace},
        {'}', TokenType::ClosingBrace},
        {'+', TokenType::Plus},
        {'-', TokenType::Minus},
        {'*', TokenType::Asterisk},
        {'/', TokenType::Slash},
        {'!', TokenType::Bang},
        {',', TokenType::Comma},
        {':', TokenType::Colon},
        {';', TokenType::Semicolon},
        {'=', TokenType::Equals},
        {'$', TokenType::Dollar},
        {'&', TokenType::Ampersand},
    };

    two_character_operators_ = {
        {"->", TokenType::Arrow},
        {"==", TokenType::EqualsEquals},
        {"!=", TokenType::BangEquals},
        {"&&", TokenType::AmpersandAmpersand},
        {"||", TokenType::PipePipe},
    };

    for (auto c : single_character_operators_ | std::views::keys)
    {
        operator_characters_.insert(c);
    }
    for (auto s : two_character_operators_ | std::views::keys)
    {
        operator_characters_.insert(s[0]);
    }

    escape_sequences_ = {
        {'\\', "\\"},
        {'"', "\""},
        {'n', "\n"},
        {'t', "\t"},
    };

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
        throw LexerError(std::format("String literal must begin with '\"', got {} instead.", current_));
    }
    Read();
    while (current_ != '"')
    {
        if (current_ == '\\')
        {
            Read();
            char escape_character{current_};
            if (escape_sequences_.contains(escape_character))
            {
                string += escape_sequences_.at(escape_character);
                Read();
            }
            else
            {
                throw LexerError(std::format("Unknown escape sequence '{}'.", current_));
            }
        }
        else
        {
            string += current_;
            Read();
        }
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
