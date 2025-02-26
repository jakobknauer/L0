#ifndef L0_LEXING_LEXER_H
#define L0_LEXING_LEXER_H

#include <istream>
#include <unordered_set>
#include <vector>

#include "l0/lexing/token.h"

namespace l0
{

std::vector<Token> Tokenize(std::istream& input);

class LexerError
{
   public:
    LexerError(std::string message);
    std::string GetMessage() const;

   private:
    const std::string message_;
};

namespace detail
{

class Lexer
{
   public:
    Lexer(std::istream& input);
    std::vector<Token> GetTokens();

   private:
    bool AtEnd() const;
    Token Next();

    char Read();
    char Skip();
    char ReadAndSkip();

    Token ReadIdentifierOrKeyword();
    Token ReadIntegerLiteral();
    Token ReadCharacterLiteral();
    Token ReadStringLiteral();

    char current_{};
    std::istream& input_;

    std::unordered_set<char> operator_characters_;
};

bool IsValidFirstIdentifierCharacter(char c);

bool IsValidIdentifierCharacter(char c);

}  // namespace detail

}  // namespace l0

#endif
