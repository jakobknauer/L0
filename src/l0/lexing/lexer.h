#ifndef L0_LEXING_LEXER_H
#define L0_LEXING_LEXER_H

#include <istream>
#include <memory>
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
    Lexer(std::shared_ptr<std::istream> input);
    std::vector<Token> GetTokens();

   private:
    bool AtEnd() const;
    Token Next();

    char Read();
    char Skip();
    char ReadAndSkip();

    Token ReadIdentifierOrKeyword();
    Token ReadIntegerLiteral();
    Token ReadStringLiteral();

    char current_{};
    std::shared_ptr<std::istream> input_;

    std::unordered_map<char, TokenType> single_character_operators_;
    std::unordered_map<std::string, TokenType> two_character_operators_;
    std::unordered_set<char> operator_characters_;
    std::unordered_set<std::string> keywords_;
    std::unordered_map<char, std::string> escape_sequences_;
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
