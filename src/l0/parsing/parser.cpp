#include "l0/parsing/parser.h"

#include <optional>

namespace l0
{

Parser::Parser(const std::vector<Token>& tokens) : tokens_{tokens} {}

std::unique_ptr<Module> Parser::Parse() { return ParseModule(); }

Token Parser::Peek()
{
    if (pos_ >= tokens_.size())
    {
        throw ParserError(std::format("Unexpectedly reached end of token stream (Peek)."));
    }
    return tokens_.at(pos_);
}

Token Parser::PeekNext()
{
    if (pos_ + 1 >= tokens_.size())
    {
        throw ParserError(std::format("Unexpectedly reached end of token stream (Peek)."));
    }
    return tokens_.at(pos_ + 1);
}

bool Parser::PeekIsKeyword(std::string_view keyword)
{
    if (pos_ >= tokens_.size())
    {
        throw ParserError(std::format("Unexpectedly reached end of token stream (PeekIfKeyword)."));
    }
    const Token& token = tokens_.at(pos_);
    return token.type == TokenType::Keyword && std::any_cast<std::string>(token.data) == keyword;
}

Token Parser::Consume()
{
    if (pos_ >= tokens_.size())
    {
        throw ParserError(std::format("Unexpectedly reached end of token stream (Consume)."));
    }
    return tokens_.at(pos_++);
}

bool Parser::ConsumeIf(TokenType type) { return ConsumeIf({type}).has_value(); }

std::optional<Token> Parser::ConsumeIf(std::initializer_list<TokenType> types)
{
    if (pos_ >= tokens_.size())
    {
        return std::nullopt;
    }
    Token token = tokens_.at(pos_);
    if (std::ranges::find(types, token.type) != types.end())
    {
        ++pos_;
        return token;
    }
    return std::nullopt;
}

Token Parser::Expect(TokenType type)
{
    if (pos_ >= tokens_.size())
    {
        throw ParserError(std::format("Expected token of type {}, reached end of token stream instead.", str(type)));
    }
    Token token = tokens_.at(pos_++);
    if (token.type != type)
    {
        throw ParserError(std::format(
            "Expected token of type {}, got token '{}' of type {} instead.", str(type), token.lexeme, str(token.type)
        ));
    }
    return token;
}

Token Parser::ExpectKeyword(std::string_view keyword)
{
    if (pos_ >= tokens_.size())
    {
        throw ParserError(std::format("Expected the keyword '{}', reached end of token stream instead.", keyword));
    }
    Token token = tokens_.at(pos_++);
    if (token.type != TokenType::Keyword)
    {
        throw ParserError(std::format(
            "Expected the keyword '{}', got token '{}' of type {} instead.", keyword, token.lexeme, str(token.type)
        ));
    }
    if (std::any_cast<std::string>(token.data) != keyword)
    {
        throw ParserError(std::format(
            "Expected the keyword '{}', got the keyword '{}' instead.",
            keyword,
            token.lexeme,
            std::any_cast<std::string>(token.data)
        ));
    }
    return token;
}

Token Parser::ConsumeAll(TokenType type)
{
    while (ConsumeIf(type))
    {
        ;
    }
    return Peek();
}

std::unique_ptr<Module> Parser::ParseModule()
{
    auto module = std::make_unique<Module>();
    module->statements = ParseStatementBlock(TokenType::EndOfFile);
    return module;
}

std::unique_ptr<StatementBlock> Parser::ParseStatementBlock(TokenType delimiter)
{
    auto block = std::make_unique<StatementBlock>();
    while (ConsumeAll(TokenType::Semicolon).type != delimiter)
    {
        auto statement = ParseStatement();
        Expect(TokenType::Semicolon);
        block->push_back(std::move(statement));
    }
    return block;
}

std::unique_ptr<Statement> Parser::ParseStatement()
{
    if (Peek().type == TokenType::Identifier && PeekNext().type == TokenType::Colon)
    {
        return ParseDeclaration();
    }
    else if (PeekIsKeyword("return"))
    {
        return ParseReturnStatement();
    }
    else if (PeekIsKeyword("if"))
    {
        return ParseConditionalStatement();
    }
    else if (PeekIsKeyword("while"))
    {
        return ParseWhileLoop();
    }
    return ParseExpressionStatement();
}

std::unique_ptr<Statement> Parser::ParseDeclaration()
{
    auto identifier = Expect(TokenType::Identifier);
    Expect(TokenType::Colon);
    auto annotation = ParseTypeAnnotation();
    Expect(TokenType::Equals);
    auto initializer = ParseExpression();
    return std::make_unique<Declaration>(
        std::any_cast<std::string>(identifier.data), std::move(annotation), std::move(initializer)
    );
}

std::unique_ptr<Statement> Parser::ParseExpressionStatement()
{
    auto expression = ParseExpression();
    return std::make_unique<ExpressionStatement>(std::move(expression));
}

std::unique_ptr<Statement> Parser::ParseReturnStatement()
{
    ExpectKeyword("return");
    if (Peek().type == TokenType::Semicolon)
    {
        return std::make_unique<ReturnStatement>(std::make_unique<UnitLiteral>());
    }
    else
    {
        auto return_value = ParseExpression();
        return std::make_unique<ReturnStatement>(std::move(return_value));
    }
}

std::unique_ptr<Statement> Parser::ParseConditionalStatement()
{
    ExpectKeyword("if");
    auto condition = ParseExpression();
    Expect(TokenType::Colon);
    Expect(TokenType::OpeningBrace);
    auto if_block = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);

    if (!PeekIsKeyword("else"))
    {
        return std::make_unique<ConditionalStatement>(std::move(condition), std::move(if_block));
    }

    Consume();
    Expect(TokenType::Colon);
    Expect(TokenType::OpeningBrace);
    auto else_block = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);
    return std::make_unique<ConditionalStatement>(std::move(condition), std::move(if_block), std::move(else_block));
}

std::unique_ptr<Statement> Parser::ParseWhileLoop()
{
    ExpectKeyword("while");
    auto condition = ParseExpression();
    Expect(TokenType::Colon);
    Expect(TokenType::OpeningBrace);
    auto body = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);
    return std::make_unique<WhileLoop>(std::move(condition), std::move(body));
}

std::unique_ptr<Expression> Parser::ParseExpression()
{
    if (ConsumeIf(TokenType::OpeningParen))
    {
        auto expression = ParseExpression();
        Expect(TokenType::ClosingParen);
        return expression;
    }
    return ParseAssignment();
}

std::unique_ptr<Expression> Parser::ParseAssignment()
{
    auto target = ParseDisjunction();
    if (ConsumeIf(TokenType::Equals))
    {
        auto value = ParseAssignment();
        return std::make_unique<Assignment>(std::move(target), std::move(value));
    }
    return target;
}

std::unique_ptr<Expression> Parser::ParseDisjunction()
{
    auto expression = ParseConjunction();
    while (ConsumeIf(TokenType::PipePipe))
    {
        expression =
            std::make_unique<BinaryOp>(std::move(expression), ParseConjunction(), BinaryOp::Operator::PipePipe);
    }
    return expression;
}

std::unique_ptr<Expression> Parser::ParseConjunction()
{
    auto expression = ParseEquality();
    while (ConsumeIf(TokenType::AmpersandAmpersand))
    {
        expression =
            std::make_unique<BinaryOp>(std::move(expression), ParseEquality(), BinaryOp::Operator::AmpersandAmpersand);
    }
    return expression;
}

std::unique_ptr<Expression> Parser::ParseEquality()
{
    auto expression = ParseSum();
    std::optional<Token> token;
    while ((token = ConsumeIf({TokenType::EqualsEquals, TokenType::BangEquals})))
    {
        BinaryOp::Operator op = (token.value().type == TokenType::EqualsEquals) ? BinaryOp::Operator::EqualsEquals
                                                                                : BinaryOp::Operator::BangEquals;
        expression = std::make_unique<BinaryOp>(std::move(expression), ParseSum(), op);
    }
    return expression;
}

std::unique_ptr<Expression> Parser::ParseSum()
{
    auto expression = ParseTerm();
    std::optional<Token> token;
    while ((token = ConsumeIf({TokenType::Plus, TokenType::Minus})))
    {
        BinaryOp::Operator op =
            (token.value().type == TokenType::Plus) ? BinaryOp::Operator::Plus : BinaryOp::Operator::Minus;
        expression = std::make_unique<BinaryOp>(std::move(expression), ParseTerm(), op);
    }
    return expression;
}

std::unique_ptr<Expression> Parser::ParseTerm()
{
    auto term = ParseUnary();
    while (ConsumeIf(TokenType::Asterisk))
    {
        term = std::make_unique<BinaryOp>(std::move(term), ParseFactor(), BinaryOp::Operator::Asterisk);
    }
    return term;
}

std::unique_ptr<Expression> Parser::ParseUnary()
{
    // TODO Refactor into using loop instead of recursion
    // TODO Refactor into using partial map from TokenType to UnaryOp::Operator
    Token token = Peek();
    switch (token.type)
    {
        case TokenType::Plus:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_unique<UnaryOp>(std::move(expression), UnaryOp::Operator::Plus);
        }
        case TokenType::Minus:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_unique<UnaryOp>(std::move(expression), UnaryOp::Operator::Minus);
        }
        case TokenType::Bang:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_unique<UnaryOp>(std::move(expression), UnaryOp::Operator::Bang);
        }
        case TokenType::Ampersand:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_unique<UnaryOp>(std::move(expression), UnaryOp::Operator::Ampersand);
        }
        case TokenType::Asterisk:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_unique<UnaryOp>(std::move(expression), UnaryOp::Operator::Asterisk);
        }
        default:
        {
            return ParseFactor();
        }
    }
}

std::unique_ptr<Expression> Parser::ParseFactor()
{
    Token token = Peek();

    switch (token.type)
    {
        case TokenType::OpeningParen:
        {
            return ParseExpression();
        }
        case TokenType::Identifier:
        {
            Consume();
            auto variable = std::make_unique<Variable>(std::any_cast<std::string>(token.data));
            if (Peek().type == TokenType::OpeningParen)
            {
                auto arguments = ParseArgumentList();
                return std::make_unique<Call>(std::move(variable), std::move(arguments));
            }
            else
            {
                return variable;
            }
        }
        case TokenType::IntegerLiteral:
        {
            Consume();
            return std::make_unique<IntegerLiteral>(std::any_cast<std::int64_t>(token.data));
        }
        case TokenType::StringLiteral:
        {
            Consume();
            return std::make_unique<StringLiteral>(std::any_cast<std::string>(token.data));
        }
        case TokenType::Dollar:
        {
            return ParseFunction();
        }
        case TokenType::Keyword:
        {
            std::string keyword = std::any_cast<std::string>(token.data);
            if (keyword == "true")
            {
                Consume();
                return std::make_unique<BooleanLiteral>(true);
            }
            else if (keyword == "false")
            {
                Consume();
                return std::make_unique<BooleanLiteral>(false);
            }
            else if (keyword == "unit")
            {
                Consume();
                return std::make_unique<UnitLiteral>();
            }
            // Fall through intended
        }
        default:
        {
            throw ParserError(std::format(
                "Expected identifier, literal, '!', or '(', got token '{}' of type {} instead.",
                token.lexeme,
                str(token.type)
            ));
        }
    }
}

std::unique_ptr<Expression> Parser::ParseFunction()
{
    Expect(TokenType::Dollar);
    auto parameters = ParseParameterDeclarationList();
    Expect(TokenType::Arrow);
    auto return_type = ParseTypeAnnotation();
    Expect(TokenType::OpeningBrace);
    auto statements = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);
    return std::make_unique<Function>(std::move(parameters), std::move(return_type), std::move(statements));
}

std::unique_ptr<ArgumentList> Parser::ParseArgumentList()
{
    auto arguments = std::make_unique<ArgumentList>();

    Expect(TokenType::OpeningParen);
    if (ConsumeIf(TokenType::ClosingParen))
    {
        return arguments;
    }

    do
    {
        auto argument = ParseExpression();
        arguments->push_back(std::move(argument));

        Token next = Consume();
        switch (next.type)
        {
            case TokenType::ClosingParen:
            {
                return arguments;
            }
            case TokenType::Comma:
            {
                if (ConsumeIf(TokenType::ClosingParen))
                {
                    return arguments;
                }
                else
                {
                    continue;
                }
            }
            default:
            {
                throw ParserError(
                    std::format("Expected ',' or ')', got token '{}' of type {} instead.", next.lexeme, str(next.type))
                );
            }
        }
    } while (true);

    return arguments;
}

std::unique_ptr<ParameterDeclarationList> Parser::ParseParameterDeclarationList()
{
    auto parameters = std::make_unique<ParameterDeclarationList>();

    Expect(TokenType::OpeningParen);
    if (ConsumeIf(TokenType::ClosingParen))
    {
        return parameters;
    }

    do
    {
        auto parameter = ParseParameterDeclaration();
        parameters->push_back(std::move(parameter));

        Token next = Consume();
        switch (next.type)
        {
            case TokenType::ClosingParen:
            {
                return parameters;
            }
            case TokenType::Comma:
            {
                if (ConsumeIf(TokenType::ClosingParen))
                {
                    return parameters;
                }
                else
                {
                    continue;
                }
            }
            default:
            {
                throw ParserError(
                    std::format("Expected ',' or ')', got token '{}' of type {} instead.", next.lexeme, str(next.type))
                );
            }
        }
    } while (true);
}

std::unique_ptr<ParameterDeclaration> Parser::ParseParameterDeclaration()
{
    Token name = Expect(TokenType::Identifier);
    Expect(TokenType::Colon);
    auto annotation = ParseTypeAnnotation();
    return std::make_unique<ParameterDeclaration>(std::any_cast<std::string>(name.data), std::move(annotation));
}

std::unique_ptr<TypeAnnotation> Parser::ParseTypeAnnotation()
{
    Token token = Peek();
    switch (token.type)
    {
        case TokenType::Identifier:
        {
            return ParseSimpleTypeAnnotation();
        }
        case TokenType::Ampersand:
        {
            return ParseReferenceTypeAnnotation();
        }
        case TokenType::OpeningParen:
        {
            return ParseFunctionTypeAnnotation();
        }
        default:
        {
            throw ParserError(std::format(
                "Expected identifier, '&', or '(', got token '{}' of type {} instead.", token.lexeme, str(token.type)
            ));
        }
    }
}

std::unique_ptr<TypeAnnotation> Parser::ParseSimpleTypeAnnotation()
{
    Token token = Expect(TokenType::Identifier);
    return std::make_unique<SimpleTypeAnnotation>(std::any_cast<std::string>(token.data));
}

std::unique_ptr<TypeAnnotation> Parser::ParseReferenceTypeAnnotation()
{
    Expect(TokenType::Ampersand);
    auto base_type = ParseSimpleTypeAnnotation();
    return std::make_unique<ReferenceTypeAnnotation>(std::move(base_type));
}

std::unique_ptr<TypeAnnotation> Parser::ParseFunctionTypeAnnotation()
{
    auto arguments = ParseParameterListAnnotation();
    if (ConsumeIf(TokenType::Arrow))
    {
        auto return_value = ParseTypeAnnotation();
        return std::make_unique<FunctionTypeAnnotation>(std::move(arguments), std::move(return_value));
    }
    else if (arguments->size() == 0)
    {
        return std::make_unique<SimpleTypeAnnotation>("()");
    }
    else
    {
        Token token = Peek();
        throw ParserError(std::format(
            "Expected '->' after non-empty type list, got token '{}' of type {} instead.", token.lexeme, str(token.type)
        ));
    }
}

std::unique_ptr<ParameterListAnnotation> Parser::ParseParameterListAnnotation()
{
    auto parameters = std::make_unique<ParameterListAnnotation>();

    Expect(TokenType::OpeningParen);
    if (ConsumeIf(TokenType::ClosingParen))
    {
        return parameters;
    }

    do
    {
        auto parameter = ParseTypeAnnotation();
        parameters->push_back(std::move(parameter));

        Token next = Consume();
        switch (next.type)
        {
            case TokenType::ClosingParen:
            {
                return parameters;
            }
            case TokenType::Comma:
            {
                if (ConsumeIf(TokenType::ClosingParen))
                {
                    return parameters;
                }
                else
                {
                    continue;
                }
            }
            default:
            {
                throw ParserError(
                    std::format("Expected ',' or ')', got token '{}' of type {} instead.", next.lexeme, str(next.type))
                );
            }
        }
    } while (true);
}

ParserError::ParserError(std::string message) : message_{message} {}

std::string ParserError::GetMessage() const { return message_; }

}  // namespace l0
