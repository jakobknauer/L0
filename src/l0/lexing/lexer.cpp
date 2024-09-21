#include "l0/lexing/lexer.h"

#include <format>
#include <ranges>

#include "l0/common/constants.h"

namespace l0
{

namespace
{

bool IsValidFirstIdentifierCharacter(char c)
{
    return std::isalpha(c) || c == '_';
}
bool IsValidIdentifierCharacter(char c)
{
    return std::isalpha(c) || std::isdigit(c) || c == '_';
}

}  // namespace

Lexer::Lexer(std::shared_ptr<std::istream> input)
    : input_{input},
      keywords_{
          Keyword::Constant,
          Keyword::Delete,
          Keyword::Else,
          Keyword::False,
          Keyword::Function,
          Keyword::If,
          Keyword::Method,
          Keyword::Mutable,
          Keyword::New,
          Keyword::Return,
          Keyword::Structure,
          Keyword::True,
          Keyword::Type,
          Keyword::UnitLiteral,
          Keyword::While,
      }
{
    single_character_operators_ = {
        {'(', TokenType::OpeningParen},
        {')', TokenType::ClosingParen},
        {'[', TokenType::OpeningBracket},
        {']', TokenType::ClosingBracket},
        {'{', TokenType::OpeningBrace},
        {'}', TokenType::ClosingBrace},
        {'+', TokenType::Plus},
        {'-', TokenType::Minus},
        {'*', TokenType::Asterisk},
        {'/', TokenType::Slash},
        {'%', TokenType::Percent},
        {'!', TokenType::Bang},
        {'.', TokenType::Dot},
        {',', TokenType::Comma},
        {':', TokenType::Colon},
        {';', TokenType::Semicolon},
        {'=', TokenType::Equals},
        {'$', TokenType::Dollar},
        {'&', TokenType::Ampersand},
        {'^', TokenType::Caret},
        {'<', TokenType::Less},
        {'>', TokenType::Greater},
    };

    two_character_operators_ = {
        {"->", TokenType::Arrow},
        {"==", TokenType::EqualsEquals},
        {"!=", TokenType::BangEquals},
        {"&&", TokenType::AmpersandAmpersand},
        {"||", TokenType::PipePipe},
        {":=", TokenType::ColonEquals},
        {"<=", TokenType::LessEquals},
        {">=", TokenType::GreaterEquals},
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
        {'\\', '\\'},
        {'"', '\"'},
        {'\'', '\''},
        {'n', '\n'},
        {'t', '\t'},
        {'0', '\0'},
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

bool Lexer::AtEnd() const
{
    return input_->eof();
}

char Lexer::Read()
{
    return current_ = input_->get();
}

char Lexer::Skip()
{
    while (current_ == ' ' || current_ == '\n')
    {
        current_ = input_->get();
    }
    if (current_ == '#')
    {
        while (current_ != '\n')
        {
            current_ = input_->get();
        }
        return Skip();
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

    if (IsValidFirstIdentifierCharacter(current_))
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

    if (current_ == '\'')
    {
        return ReadCharacterLiteral();
    }

    throw LexerError(std::format("Unexpected character '{}'.", current_));
}

Token Lexer::ReadIdentifierOrKeyword()
{
    if (!IsValidFirstIdentifierCharacter(current_))
    {
        throw LexerError(std::format("Invalid first character of identifier: '{}'.", current_));
    }

    std::string lexeme{};
    while (IsValidIdentifierCharacter(current_))
    {
        lexeme.append(std::string{current_});
        Read();
    }
    Skip();

    bool is_keyword{keywords_.contains(lexeme)};
    return Token{
        .type = is_keyword ? TokenType::Keyword : TokenType::Identifier,
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

Token Lexer::ReadCharacterLiteral()
{
    if (current_ != '\'')
    {
        throw LexerError(std::format("Character literal must begin with single quotes ('), got {} instead.", current_));
    }
    Read();

    char8_t character = current_;
    if (character == '\\')
    {
        Read();
        char8_t escape_character = current_;
        character = escape_sequences_.at(escape_character);
    }
    Read();

    if (current_ != '\'')
    {
        throw LexerError(std::format("Character literal must end with single quotes ('), got {} instead.", current_));
    }
    Read();
    Skip();
    return Token{
        .type = TokenType::CharacterLiteral,
        .lexeme = std::format("'{}'", character),
        .data = character,
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

LexerError::LexerError(std::string message)
    : message_{message}
{
}

std::string LexerError::GetMessage() const
{
    return message_;
}

}  // namespace l0
