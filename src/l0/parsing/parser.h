#ifndef L0_PARSING_PARSING_H
#define L0_PARSING_PARSING_H

#include <memory>
#include <vector>

#include "l0/ast/expression.h"
#include "l0/ast/module.h"
#include "l0/ast/statement.h"
#include "l0/ast/type_annotation.h"
#include "l0/ast/type_expression.h"
#include "l0/lexing/token.h"

namespace l0
{

std::unique_ptr<Module> Parse(const std::vector<Token>& tokens);

class ParserError
{
   public:
    ParserError(std::string message);
    std::string GetMessage() const;

   private:
    const std::string message_;
};

namespace detail
{

class Parser
{
   public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Module> Parse();

   private:
    const std::vector<Token>& tokens_;
    std::size_t pos_{0};
    Identifier current_namespace_{};

    Token Peek();
    Token PeekNext();
    bool PeekIsKeyword(std::string_view keyword);
    Token Consume();
    bool ConsumeIf(TokenType type);
    std::optional<Token> ConsumeIf(std::initializer_list<TokenType> type);
    bool ConsumeIfKeyword(std::string_view keyword);
    std::optional<std::string> ConsumeIfKeyword(std::initializer_list<std::string_view> keywords);
    Token ConsumeAll(TokenType type);
    Token Expect(TokenType type);
    Token Expect(std::initializer_list<TokenType> types);
    Token ExpectKeyword(std::string_view keyword);

    void ParseNamespaceStatementBlock(TokenType delimiter, Module& module);
    void ParseGlobalStatement(Module& module);

    std::shared_ptr<StatementBlock> ParseStatementBlock(TokenType delimiter);
    std::shared_ptr<Statement> ParseStatement();
    std::shared_ptr<Statement> ParseDeclaration();
    std::variant<std::shared_ptr<Declaration>, std::shared_ptr<TypeDeclaration>> ParseGlobalDeclaration();
    std::shared_ptr<Statement> ParseUnannotatedDeclaration();
    std::shared_ptr<Statement> ParseExpressionStatement();
    std::shared_ptr<Statement> ParseReturnStatement();
    std::shared_ptr<Statement> ParseConditionalStatement();
    std::shared_ptr<Statement> ParseWhileLoop();
    std::shared_ptr<Statement> ParseDeallocation();

    std::shared_ptr<Expression> ParseExpression();
    std::shared_ptr<Expression> ParseAssignment();
    std::shared_ptr<Expression> ParseDisjunction();
    std::shared_ptr<Expression> ParseConjunction();
    std::shared_ptr<Expression> ParseEquality();
    std::shared_ptr<Expression> ParseComparison();
    std::shared_ptr<Expression> ParseSum();
    std::shared_ptr<Expression> ParseTerm();
    std::shared_ptr<Expression> ParseUnary();
    std::shared_ptr<Expression> ParseFactor();
    std::shared_ptr<Expression> ParseCallsDerefsAndMemberAccessors();
    std::shared_ptr<Expression> ParseAtomicExpression();
    std::shared_ptr<Expression> ParseFunction();
    std::shared_ptr<Expression> ParseAllocation();

    std::shared_ptr<ArgumentList> ParseArgumentList();
    std::shared_ptr<ParameterDeclarationList> ParseParameterDeclarationList();
    std::shared_ptr<CaptureList> ParseCaptureList();
    std::shared_ptr<ParameterDeclaration> ParseParameterDeclaration();
    std::shared_ptr<MemberInitializerList> ParseMemberInitializerList();

    std::shared_ptr<TypeAnnotation> ParseTypeAnnotation();
    std::shared_ptr<TypeAnnotation> TryParseUnqualifiedTypeAnnotation();
    std::shared_ptr<TypeAnnotation> ParseSimpleTypeAnnotation();
    std::shared_ptr<TypeAnnotation> ParseReferenceTypeAnnotation();
    std::shared_ptr<TypeAnnotation> ParseFunctionTypeAnnotation();
    std::shared_ptr<TypeAnnotation> ParseMethodTypeAnnotation();
    std::shared_ptr<ParameterListAnnotation> ParseParameterListAnnotation();

    std::shared_ptr<TypeExpression> ParseStruct();
    std::shared_ptr<StructMemberDeclarationList> ParseStructMemberDeclarationList();
    std::shared_ptr<TypeExpression> ParseEnum();
    std::shared_ptr<EnumMemberDeclarationList> ParseEnumMemberDeclarationList();

    std::shared_ptr<Declaration> ParseAlternativeFunctionDeclaration();
    std::shared_ptr<TypeDeclaration> ParseAlternativeStructDeclaration();
    std::shared_ptr<TypeDeclaration> ParseAlternativeEnumDeclaration();
    std::shared_ptr<Declaration> ParseAlternativeMethodDeclaration();

    Identifier ParseIdentifier();
};

}  // namespace detail

}  // namespace l0

#endif
