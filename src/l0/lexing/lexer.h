#ifndef L0_LEXING_LEXER_H
#define L0_LEXING_LEXER_H

#include <istream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "l0/lexing/token.h"

namespace l0
{

class ILexer
{
   public:
    virtual ~ILexer() = default;
    virtual std::vector<Token> GetTokens() = 0;
};

class Lexer : public ILexer
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

    std::unordered_map<char, TokenType> single_character_operators_;
    std::unordered_map<std::string, TokenType> two_character_operators_;
    std::unordered_set<char> operator_characters_;
    std::unordered_set<std::string_view> keywords_;
    std::unordered_map<char, char> escape_sequences_;
};

class LexerError
{
   public:
    LexerError(std::string message);
    std::string GetMessage() const;

   private:
    const std::string message_;
};

}  // namespace l0

#endif
