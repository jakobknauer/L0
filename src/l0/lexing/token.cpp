#include "l0/lexing/token.h"

#include <utility>

namespace l0
{

std::string str(TokenType type)
{
    switch (type)
    {
        case TokenType::None:
            return "None";
        case TokenType::EndOfFile:
            return "EOF";
        case TokenType::Keyword:
            return "Keyword";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::IntegerLiteral:
            return "IntegerLiteral";
        case TokenType::CharacterLiteral:
            return "CharacterLiteral";
        case TokenType::StringLiteral:
            return "StringLiteral";
        case TokenType::Plus:
            return "Plus";
        case TokenType::Minus:
            return "Minus";
        case TokenType::Asterisk:
            return "Asterisk";
        case TokenType::Slash:
            return "Slash";
        case TokenType::Percent:
            return "Percent";
        case TokenType::Bang:
            return "Bang";
        case TokenType::Ampersand:
            return "Ampersand";
        case TokenType::Caret:
            return "Caret";
        case TokenType::EqualsEquals:
            return "EqualsEquals";
        case TokenType::BangEquals:
            return "BangEquals";
        case TokenType::AmpersandAmpersand:
            return "AmpersandAmpersand";
        case TokenType::PipePipe:
            return "PipePipe";
        case TokenType::Less:
            return "Less";
        case TokenType::Greater:
            return "Greater";
        case TokenType::LessEquals:
            return "LessEquals";
        case TokenType::GreaterEquals:
            return "GreaterEquals";
        case TokenType::OpeningParen:
            return "OpeningParen";
        case TokenType::ClosingParen:
            return "ClosingParen";
        case TokenType::OpeningBracket:
            return "OpeningBracket";
        case TokenType::ClosingBracket:
            return "ClosingBracket";
        case TokenType::OpeningBrace:
            return "OpeningBrace";
        case TokenType::ClosingBrace:
            return "ClosingBrace";
        case TokenType::Dot:
            return "Dot";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Colon:
            return "Colon";
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::Equals:
            return "Equals";
        case TokenType::Arrow:
            return "Arrow";
        case l0::TokenType::Dollar:
            return "Dollar";
        case l0::TokenType::ColonEquals:
            return "ColonEquals";
        case l0::TokenType::ColonColon:
            return "ColonColon";
    }
    std::unreachable();
}

}  // namespace l0
