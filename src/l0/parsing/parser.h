#ifndef L0_PARSING_PARSING_H
#define L0_PARSING_PARSING_H

#include <memory>
#include <vector>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"
#include "l0/lexing/token.h"

namespace l0
{

class IParser
{
   public:
    virtual ~IParser() = default;
    virtual std::unique_ptr<Module> Parse() = 0;
};

class Parser : public IParser
{
   public:
    Parser(const std::vector<Token>& tokens);
    virtual std::unique_ptr<Module> Parse() override;

   private:
    const std::vector<Token>& tokens_;
    std::size_t pos_{0};

    Token Peek();
    Token PeekNext();
    bool PeekIsKeyword(std::string_view keyword);
    Token Consume();
    bool ConsumeIf(TokenType type);
    std::optional<Token> ConsumeIf(std::initializer_list<TokenType> type);
    Token ConsumeAll(TokenType type);
    Token Expect(TokenType type);
    Token Expect(std::initializer_list<TokenType> types);
    Token ExpectKeyword(std::string_view keyword);

    std::unique_ptr<Module> ParseModule();
    std::unique_ptr<StatementBlock> ParseStatementBlock(TokenType delimiter);
    std::unique_ptr<Statement> ParseStatement();
    std::unique_ptr<Statement> ParseDeclaration();
    std::unique_ptr<Statement> ParseExpressionStatement();
    std::unique_ptr<Statement> ParseReturnStatement();
    std::unique_ptr<Statement> ParseConditionalStatement();
    std::unique_ptr<Statement> ParseWhileLoop();
    std::unique_ptr<Expression> ParseExpression();
    std::unique_ptr<Expression> ParseAssignment();
    std::unique_ptr<Expression> ParseDisjunction();
    std::unique_ptr<Expression> ParseConjunction();
    std::unique_ptr<Expression> ParseEquality();
    std::unique_ptr<Expression> ParseSum();
    std::unique_ptr<Expression> ParseTerm();
    std::unique_ptr<Expression> ParseUnary();
    std::unique_ptr<Expression> ParseFactor();
    std::unique_ptr<Expression> ParseFunction();
    std::unique_ptr<ArgumentList> ParseArgumentList();
    std::unique_ptr<ParameterDeclarationList> ParseParameterDeclarationList();
    std::unique_ptr<ParameterDeclaration> ParseParameterDeclaration();
    std::unique_ptr<TypeAnnotation> ParseTypeAnnotation();
    std::unique_ptr<TypeAnnotation> ParseSimpleTypeAnnotation();
    std::unique_ptr<TypeAnnotation> ParseReferenceTypeAnnotation();
    std::unique_ptr<TypeAnnotation> ParseFunctionTypeAnnotation();
    std::unique_ptr<ParameterListAnnotation> ParseParameterListAnnotation();
};

class ParserError
{
   public:
    ParserError(std::string message);
    std::string GetMessage() const;

   private:
    const std::string message_;
};

}  // namespace l0

#endif
