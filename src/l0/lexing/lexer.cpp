#include "l0/lexing/lexer.h"

#include <format>
#include <ranges>
#include <unordered_map>

#include "l0/common/constants.h"

namespace l0
{

std::vector<Token> Tokenize(std::istream& input)
{
    return detail::Lexer{input}.GetTokens();
}

LexerError::LexerError(std::string message)
    : message_{message}
{
}

std::string LexerError::GetMessage() const
{
    return message_;
}

namespace detail
{

static const std::unordered_map<char, TokenType> SINGLE_CHARACTER_TOKENS{
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

static const std::unordered_map<std::string, TokenType> TWO_CHARACTER_TOKENS{
    {"->", TokenType::Arrow},
    {"==", TokenType::EqualsEquals},
    {"!=", TokenType::BangEquals},
    {"&&", TokenType::AmpersandAmpersand},
    {"||", TokenType::PipePipe},
    {":=", TokenType::ColonEquals},
    {"<=", TokenType::LessEquals},
    {">=", TokenType::GreaterEquals},
    {"::", TokenType::ColonColon},
};

static const std::unordered_set<std::string_view> KEYWORDS{
    Keyword::Constant,
    Keyword::Delete,
    Keyword::Else,
    Keyword::Enumeration,
    Keyword::False,
    Keyword::Function,
    Keyword::If,
    Keyword::Method,
    Keyword::Mutable,
    Keyword::Namespace,
    Keyword::New,
    Keyword::Return,
    Keyword::Structure,
    Keyword::True,
    Keyword::Type,
    Keyword::UnitLiteral,
    Keyword::While,
};

static const std::unordered_map<char, char> ESCAPE_SEQUENCES{
    {'\\', '\\'},
    {'"', '\"'},
    {'\'', '\''},
    {'n', '\n'},
    {'t', '\t'},
    {'0', '\0'},
};

Lexer::Lexer(std::istream& input)
    : input_{input}
{
    for (auto c : SINGLE_CHARACTER_TOKENS | std::views::keys)
    {
        operator_characters_.insert(c);
    }
    for (auto s : TWO_CHARACTER_TOKENS | std::views::keys)
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

bool Lexer::AtEnd() const
{
    return input_.eof();
}

char Lexer::Read()
{
    return current_ = input_.get();
}

char Lexer::Skip()
{
    while (current_ == ' ' || current_ == '\n')
    {
        current_ = input_.get();
    }
    if (current_ == '#')
    {
        while (current_ != '\n')
        {
            current_ = input_.get();
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

        if (TWO_CHARACTER_TOKENS.contains(c1c2))
        {
            type = TWO_CHARACTER_TOKENS.at(c1c2);
            lexeme = c1c2;
            ReadAndSkip();
        }
        else if (SINGLE_CHARACTER_TOKENS.contains(c1))
        {
            type = SINGLE_CHARACTER_TOKENS.at(c1);
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

    if (detail::IsValidFirstIdentifierCharacter(current_))
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
    if (!detail::IsValidFirstIdentifierCharacter(current_))
    {
        throw LexerError(std::format("Invalid first character of identifier: '{}'.", current_));
    }

    std::string lexeme{};
    while (detail::IsValidIdentifierCharacter(current_))
    {
        lexeme.append(std::string{current_});
        Read();
    }
    Skip();

    bool is_keyword{KEYWORDS.contains(lexeme)};
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
        character = ESCAPE_SEQUENCES.at(escape_character);
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
        .lexeme = std::format("'{}'", (char)character),
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
            if (ESCAPE_SEQUENCES.contains(escape_character))
            {
                string += ESCAPE_SEQUENCES.at(escape_character);
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

bool IsValidFirstIdentifierCharacter(char c)
{
    return std::isalpha(c) || c == '_';
}

bool IsValidIdentifierCharacter(char c)
{
    return std::isalpha(c) || std::isdigit(c) || c == '_';
}

}  // namespace detail

}  // namespace l0
