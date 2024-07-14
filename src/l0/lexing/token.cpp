#include "l0/lexing/token.h"

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
        case TokenType::Bang:
            return "Bang";
        case TokenType::Ampersand:
            return "Ampersand";
        case TokenType::EqualsEquals:
            return "EqualsEquals";
        case TokenType::BangEquals:
            return "BangEquals";
        case TokenType::AmpersandAmpersand:
            return "AmpersandAmpersand";
        case TokenType::PipePipe:
            return "PipePipe";
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
    }
}

}  // namespace l0
