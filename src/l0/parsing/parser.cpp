#include "l0/parsing/parser.h"

#include <optional>
#include <ranges>

#include "l0/common/constants.h"

namespace l0
{

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_{tokens}
{
}

std::shared_ptr<Module> Parser::Parse()
{
    return ParseModule();
}

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
        throw ParserError(std::format("Unexpectedly reached end of token stream (PeekNext)."));
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

bool Parser::ConsumeIf(TokenType type)
{
    return ConsumeIf({type}).has_value();
}

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

bool Parser::ConsumeIfKeyword(std::string_view keyword)
{
    return ConsumeIfKeyword({keyword}).has_value();
}

std::optional<std::string> Parser::ConsumeIfKeyword(std::initializer_list<std::string_view> keywords)
{
    if (pos_ >= tokens_.size())
    {
        return std::nullopt;
    }
    const Token& token = tokens_.at(pos_);
    if (token.type != TokenType::Keyword)
    {
        return std::nullopt;
    }
    std::string keyword = std::any_cast<std::string>(token.data);
    if (std::ranges::find(keywords, keyword) != keywords.end())
    {
        ++pos_;
        return keyword;
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

Token Parser::Expect(std::initializer_list<TokenType> types)
{
    if (pos_ >= tokens_.size())
    {
        throw ParserError(std::format("Expected token of types {}, reached end of token stream instead.", str(types)));
    }
    Token token = tokens_.at(pos_++);
    if (std::ranges::find(types, token.type) != types.end())
    {
        return token;
    }
    throw ParserError(std::format(
        "Expected token of types {}, got token '{}' of type {} instead.", str(types), token.lexeme, str(token.type)
    ));
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

std::shared_ptr<Module> Parser::ParseModule()
{
    auto module = std::make_shared<Module>();
    module->statements = ParseStatementBlock(TokenType::EndOfFile);
    return module;
}

std::shared_ptr<StatementBlock> Parser::ParseStatementBlock(TokenType delimiter)
{
    auto block = std::make_shared<StatementBlock>();
    while (ConsumeAll(TokenType::Semicolon).type != delimiter)
    {
        auto statement = ParseStatement();
        Expect(TokenType::Semicolon);
        block->push_back(statement);
    }
    return block;
}

std::shared_ptr<Statement> Parser::ParseStatement()
{
    if (Peek().type == TokenType::Identifier && PeekNext().type == TokenType::Colon)
    {
        return ParseDeclaration();
    }
    else if (Peek().type == TokenType::Identifier && PeekNext().type == TokenType::ColonEquals)
    {
        return ParseUnannotatedDeclaration();
    }
    else if (PeekIsKeyword(Keyword::Return))
    {
        return ParseReturnStatement();
    }
    else if (PeekIsKeyword(Keyword::If))
    {
        return ParseConditionalStatement();
    }
    else if (PeekIsKeyword(Keyword::While))
    {
        return ParseWhileLoop();
    }
    else if (PeekIsKeyword(Keyword::Delete))
    {
        return ParseDeallocation();
    }
    else if (PeekIsKeyword(Keyword::Function))
    {
        return ParseAlternativeFunctionDeclaration();
    }
    else if (PeekIsKeyword(Keyword::Structure))
    {
        return ParseAlternativeStructDeclaration();
    }
    else if (PeekIsKeyword(Keyword::Method))
    {
        return ParseAlternativeMethodDeclaration();
    }
    return ParseExpressionStatement();
}

std::shared_ptr<Statement> Parser::ParseDeclaration()
{
    auto identifier = Expect(TokenType::Identifier);
    Expect(TokenType::Colon);
    if (ConsumeIfKeyword(Keyword::Type))
    {
        Expect(TokenType::Equals);
        auto definition = ParseStruct();
        return std::make_shared<TypeDeclaration>(std::any_cast<std::string>(identifier.data), definition);
    }
    else
    {
        auto annotation = ParseTypeAnnotation();
        std::shared_ptr<Expression> initializer{nullptr};
        if (ConsumeIf(TokenType::Equals))
        {
            initializer = ParseExpression();
        }
        return std::make_shared<Declaration>(std::any_cast<std::string>(identifier.data), annotation, initializer);
    }
}

std::shared_ptr<Statement> Parser::ParseUnannotatedDeclaration()
{
    auto identifier = Expect(TokenType::Identifier);
    Expect(TokenType::ColonEquals);
    auto initializer = ParseExpression();
    return std::make_shared<Declaration>(std::any_cast<std::string>(identifier.data), initializer);
}

std::shared_ptr<Statement> Parser::ParseExpressionStatement()
{
    auto expression = ParseExpression();
    return std::make_shared<ExpressionStatement>(expression);
}

std::shared_ptr<Statement> Parser::ParseReturnStatement()
{
    ExpectKeyword(Keyword::Return);
    if (Peek().type == TokenType::Semicolon)
    {
        return std::make_shared<ReturnStatement>(std::make_shared<UnitLiteral>());
    }
    else
    {
        auto return_value = ParseExpression();
        return std::make_shared<ReturnStatement>(return_value);
    }
}

std::shared_ptr<Statement> Parser::ParseConditionalStatement()
{
    ExpectKeyword(Keyword::If);
    auto condition = ParseExpression();
    Expect(TokenType::Colon);
    Expect(TokenType::OpeningBrace);
    auto if_block = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);

    if (!PeekIsKeyword(Keyword::Else))
    {
        return std::make_shared<ConditionalStatement>(condition, if_block);
    }

    Consume();
    if (ConsumeIf(TokenType::Colon))
    {
        Expect(TokenType::OpeningBrace);
        auto else_block = ParseStatementBlock(TokenType::ClosingBrace);
        Expect(TokenType::ClosingBrace);
        return std::make_shared<ConditionalStatement>(condition, if_block, else_block);
    }
    else if (PeekIsKeyword(Keyword::If))
    {
        auto else_block = std::make_shared<StatementBlock>();
        auto else_if = ParseConditionalStatement();
        else_block->push_back(else_if);
        return std::make_shared<ConditionalStatement>(condition, if_block, else_block);
    }
    else
    {
        throw ParserError(std::format(
            "Expected ':' or 'if' after 'else', got token '{}' of type '{}' instead", Peek().lexeme, str(Peek().type)
        ));
    }
}

std::shared_ptr<Statement> Parser::ParseWhileLoop()
{
    ExpectKeyword(Keyword::While);
    auto condition = ParseExpression();
    Expect(TokenType::Colon);
    Expect(TokenType::OpeningBrace);
    auto body = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);
    return std::make_shared<WhileLoop>(condition, body);
}

std::shared_ptr<Statement> Parser::ParseDeallocation()
{
    ExpectKeyword(Keyword::Delete);
    auto operand = ParseExpression();
    return std::make_shared<Deallocation>(operand);
}

std::shared_ptr<Expression> Parser::ParseExpression()
{
    return ParseAssignment();
}

std::shared_ptr<Expression> Parser::ParseAssignment()
{
    auto target = ParseDisjunction();
    if (ConsumeIf(TokenType::Equals))
    {
        auto value = ParseAssignment();
        return std::make_shared<Assignment>(target, value);
    }
    return target;
}

std::shared_ptr<Expression> Parser::ParseDisjunction()
{
    auto expression = ParseConjunction();
    while (ConsumeIf(TokenType::PipePipe))
    {
        expression = std::make_shared<BinaryOp>(expression, ParseConjunction(), BinaryOp::Operator::PipePipe);
    }
    return expression;
}

std::shared_ptr<Expression> Parser::ParseConjunction()
{
    auto expression = ParseEquality();
    while (ConsumeIf(TokenType::AmpersandAmpersand))
    {
        expression = std::make_shared<BinaryOp>(expression, ParseEquality(), BinaryOp::Operator::AmpersandAmpersand);
    }
    return expression;
}

std::shared_ptr<Expression> Parser::ParseEquality()
{
    auto expression = ParseComparison();
    std::optional<Token> token;
    while ((token = ConsumeIf({TokenType::EqualsEquals, TokenType::BangEquals})))
    {
        BinaryOp::Operator op = (token.value().type == TokenType::EqualsEquals) ? BinaryOp::Operator::EqualsEquals
                                                                                : BinaryOp::Operator::BangEquals;
        expression = std::make_shared<BinaryOp>(expression, ParseComparison(), op);
    }
    return expression;
}

std::shared_ptr<Expression> Parser::ParseComparison()
{
    auto expression = ParseSum();
    std::optional<Token> token;
    while ((token = ConsumeIf({TokenType::Less, TokenType::Greater, TokenType::LessEquals, TokenType::GreaterEquals})))
    {
        BinaryOp::Operator op;
        switch (token.value().type)
        {
            case TokenType::Less:
            {
                op = BinaryOp::Operator::Less;
                break;
            }
            case TokenType::Greater:
            {
                op = BinaryOp::Operator::Greater;
                break;
            }
            case TokenType::LessEquals:
            {
                op = BinaryOp::Operator::LessEquals;
                break;
            }
            case TokenType::GreaterEquals:
            {
                op = BinaryOp::Operator::GreaterEquals;
                break;
            }
            default:
            {
                throw ParserError("ParseComparison()");
            }
        }
        expression = std::make_shared<BinaryOp>(expression, ParseSum(), op);
    }
    return expression;
}

std::shared_ptr<Expression> Parser::ParseSum()
{
    auto expression = ParseTerm();
    std::optional<Token> token;
    while ((token = ConsumeIf({TokenType::Plus, TokenType::Minus})))
    {
        BinaryOp::Operator op =
            (token.value().type == TokenType::Plus) ? BinaryOp::Operator::Plus : BinaryOp::Operator::Minus;
        expression = std::make_shared<BinaryOp>(expression, ParseTerm(), op);
    }
    return expression;
}

std::shared_ptr<Expression> Parser::ParseTerm()
{
    auto term = ParseUnary();
    std::optional<Token> token;
    while ((token = ConsumeIf({TokenType::Asterisk, TokenType::Slash, TokenType::Percent})))
    {
        BinaryOp::Operator op;
        switch (token.value().type)
        {
            case TokenType::Asterisk:
            {
                op = BinaryOp::Operator::Asterisk;
                break;
            }
            case TokenType::Slash:
            {
                op = BinaryOp::Operator::Slash;
                break;
            }
            case TokenType::Percent:
            {
                op = BinaryOp::Operator::Percent;
                break;
            }
            default:
            {
                throw ParserError("ParseTerm()");
            }
        }
        term = std::make_shared<BinaryOp>(term, ParseFactor(), op);
    }
    return term;
}

std::shared_ptr<Expression> Parser::ParseUnary()
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
            return std::make_shared<UnaryOp>(expression, UnaryOp::Operator::Plus);
        }
        case TokenType::Minus:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_shared<UnaryOp>(expression, UnaryOp::Operator::Minus);
        }
        case TokenType::Bang:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_shared<UnaryOp>(expression, UnaryOp::Operator::Bang);
        }
        case TokenType::Ampersand:
        {
            Consume();
            auto expression = ParseUnary();
            return std::make_shared<UnaryOp>(expression, UnaryOp::Operator::Ampersand);
        }
        default:
        {
            return ParseFactor();
        }
    }
}

std::shared_ptr<Expression> Parser::ParseFactor()
{
    if (PeekIsKeyword(Keyword::New))
    {
        return ParseAllocation();
    }
    return ParseCallsDerefsAndMemberAccessors();
}

std::shared_ptr<Expression> Parser::ParseCallsDerefsAndMemberAccessors()
{
    auto expression = ParseAtomicExpression();

    while (true)
    {
        if (Peek().type == TokenType::OpeningParen)
        {
            auto arguments = ParseArgumentList();
            expression = std::make_shared<Call>(expression, arguments);
        }
        else if (ConsumeIf(TokenType::Dot))
        {
            auto member = Expect(TokenType::Identifier);
            expression = std::make_shared<MemberAccessor>(expression, std::any_cast<std::string>(member.data));
        }
        else if (ConsumeIf(TokenType::Caret))
        {
            expression = std::make_shared<UnaryOp>(expression, UnaryOp::Operator::Caret);
        }
        else
        {
            break;
        }
    }
    return expression;
}

std::shared_ptr<Expression> Parser::ParseAtomicExpression()
{
    Token token = Peek();

    switch (token.type)
    {
        case TokenType::OpeningParen:
        {
            Consume();
            auto expression = ParseExpression();
            Expect(TokenType::ClosingParen);
            return expression;
        }
        case TokenType::Identifier:
        {
            if (PeekNext().type == TokenType::OpeningBrace)
            {
                return ParseInitializer();
            }
            else
            {
                Consume();
                auto variable = std::make_shared<Variable>(std::any_cast<std::string>(token.data));
                return variable;
            }
        }
        case TokenType::IntegerLiteral:
        {
            Consume();
            return std::make_shared<IntegerLiteral>(std::any_cast<std::int64_t>(token.data));
        }
        case TokenType::CharacterLiteral:
        {
            Consume();
            return std::make_shared<CharacterLiteral>(std::any_cast<char8_t>(token.data));
        }
        case TokenType::StringLiteral:
        {
            Consume();
            return std::make_shared<StringLiteral>(std::any_cast<std::string>(token.data));
        }
        case TokenType::Dollar:
        {
            return ParseFunction();
        }
        case TokenType::Keyword:
        {
            std::string keyword = std::any_cast<std::string>(token.data);
            if (keyword == Keyword::True)
            {
                Consume();
                return std::make_shared<BooleanLiteral>(true);
            }
            else if (keyword == Keyword::False)
            {
                Consume();
                return std::make_shared<BooleanLiteral>(false);
            }
            else if (keyword == Keyword::UnitLiteral)
            {
                Consume();
                return std::make_shared<UnitLiteral>();
            }
            [[fallthrough]];
        }
        default:
        {
            throw ParserError(std::format(
                "Expected identifier, literal, '!', or '(', got token '{}' of type '{}' instead.",
                token.lexeme,
                str(token.type)
            ));
        }
    }
}

std::shared_ptr<Expression> Parser::ParseFunction()
{
    Expect(TokenType::Dollar);
    auto parameters = ParseParameterDeclarationList();
    Expect(TokenType::Arrow);
    auto return_type = ParseTypeAnnotation();
    Expect(TokenType::OpeningBrace);
    auto statements = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);
    return std::make_shared<Function>(parameters, return_type, statements);
}

std::shared_ptr<Expression> Parser::ParseInitializer()
{
    auto annotation = ParseSimpleTypeAnnotation();
    auto member_initializer_list = ParseMemberInitializerList();

    return std::make_shared<Initializer>(annotation, member_initializer_list);
}

std::shared_ptr<Expression> Parser::ParseAllocation()
{
    ExpectKeyword(Keyword::New);

    std::shared_ptr<Expression> size{nullptr};
    if (ConsumeIf(TokenType::OpeningBracket))
    {
        size = ParseExpression();
        Expect(TokenType::ClosingBracket);
    }

    auto annotation = TryParseUnqualifiedTypeAnnotation();

    std::shared_ptr<MemberInitializerList> member_initializer_list{nullptr};
    if (Peek().type == TokenType::OpeningBrace)
    {
        member_initializer_list = ParseMemberInitializerList();
    }

    return std::make_shared<Allocation>(annotation, size, member_initializer_list);
}

std::shared_ptr<ArgumentList> Parser::ParseArgumentList()
{
    auto arguments = std::make_shared<ArgumentList>();

    Expect(TokenType::OpeningParen);
    if (ConsumeIf(TokenType::ClosingParen))
    {
        return arguments;
    }

    do
    {
        auto argument = ParseExpression();
        arguments->push_back(argument);

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
                throw ParserError(std::format(
                    "Expected ',' or ')', got token '{}' of type '{}' instead.", next.lexeme, str(next.type)
                ));
            }
        }
    } while (true);

    return arguments;
}

std::shared_ptr<ParameterDeclarationList> Parser::ParseParameterDeclarationList()
{
    auto parameters = std::make_shared<ParameterDeclarationList>();

    Expect(TokenType::OpeningParen);
    if (ConsumeIf(TokenType::ClosingParen))
    {
        return parameters;
    }

    do
    {
        auto parameter = ParseParameterDeclaration();
        parameters->push_back(parameter);

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
                throw ParserError(std::format(
                    "Expected ',' or ')', got token '{}' of type '{}' instead.", next.lexeme, str(next.type)
                ));
            }
        }
    } while (true);
}

std::shared_ptr<ParameterDeclaration> Parser::ParseParameterDeclaration()
{
    Token name = Expect(TokenType::Identifier);
    Expect(TokenType::Colon);
    auto annotation = ParseTypeAnnotation();
    return std::make_shared<ParameterDeclaration>(std::any_cast<std::string>(name.data), annotation);
}

std::shared_ptr<TypeAnnotation> Parser::ParseTypeAnnotation()
{
    auto qualifier = ConsumeIfKeyword({Keyword::Mutable, Keyword::Constant});
    auto type_annotation = TryParseUnqualifiedTypeAnnotation();

    if (!type_annotation && !qualifier)
    {
        throw ParserError(std::format(
            "Expected 'mut', 'const', or unqualified type annotation, got token '{}' of type '{}' instead.",
            Peek().lexeme,
            str(Peek().type)
        ));
    }

    if (!type_annotation)
    {
        type_annotation = std::make_shared<MutabilityOnlyTypeAnnotation>();
    }

    if (qualifier == Keyword::Mutable)
    {
        type_annotation->mutability = TypeAnnotationQualifier::Mutable;
    }
    else if (qualifier == Keyword::Constant)
    {
        type_annotation->mutability = TypeAnnotationQualifier::Constant;
    }

    return type_annotation;
}

std::shared_ptr<TypeAnnotation> Parser::TryParseUnqualifiedTypeAnnotation()
{
    Token token = Peek();
    switch (token.type)
    {
        case TokenType::Identifier:
        {
            return ParseSimpleTypeAnnotation();
        }
        case TokenType::Ampersand:
        case TokenType::AmpersandAmpersand:
        {
            return ParseReferenceTypeAnnotation();
        }
        case TokenType::OpeningParen:
        {
            return ParseFunctionTypeAnnotation();
        }
        case TokenType::Keyword:
        {
            if (PeekIsKeyword(Keyword::Method))
            {
                return ParseMethodTypeAnnotation();
            }
            [[fallthrough]];
        }
        default:
        {
            return nullptr;
        }
    }
}

std::shared_ptr<TypeAnnotation> Parser::ParseSimpleTypeAnnotation()
{
    Token token = Expect(TokenType::Identifier);
    return std::make_shared<SimpleTypeAnnotation>(std::any_cast<std::string>(token.data));
}

std::shared_ptr<TypeAnnotation> Parser::ParseReferenceTypeAnnotation()
{
    auto qualifier = Expect({TokenType::Ampersand, TokenType::AmpersandAmpersand});

    auto base_type = ParseTypeAnnotation();
    auto single_ref = std::make_shared<ReferenceTypeAnnotation>(base_type);

    if (qualifier.type == TokenType::Ampersand)
    {
        return single_ref;
    }

    auto double_ref = std::make_shared<ReferenceTypeAnnotation>(single_ref);
    return double_ref;
}

std::shared_ptr<TypeAnnotation> Parser::ParseFunctionTypeAnnotation()
{
    auto arguments = ParseParameterListAnnotation();
    if (ConsumeIf(TokenType::Arrow))
    {
        auto return_value = ParseTypeAnnotation();
        return std::make_shared<FunctionTypeAnnotation>(arguments, return_value);
    }
    else if (arguments->empty())
    {
        return std::make_shared<SimpleTypeAnnotation>(std::string{Typename::Unit});
    }
    else
    {
        Token token = Peek();
        throw ParserError(std::format(
            "Expected '->' after non-empty type list, got token '{}' of type '{}' instead.",
            token.lexeme,
            str(token.type)
        ));
    }
}

std::shared_ptr<TypeAnnotation> Parser::ParseMethodTypeAnnotation()
{
    ExpectKeyword(Keyword::Method);
    auto inner = ParseFunctionTypeAnnotation();
    auto inner_as_function_type = dynamic_pointer_cast<FunctionTypeAnnotation>(inner);
    if (!inner)
    {
        throw ParserError(std::format("Expected function type annotation after 'method'."));
    }
    return std::make_shared<MethodTypeAnnotation>(inner_as_function_type);
}

std::shared_ptr<ParameterListAnnotation> Parser::ParseParameterListAnnotation()
{
    auto parameters = std::make_shared<ParameterListAnnotation>();

    Expect(TokenType::OpeningParen);
    if (ConsumeIf(TokenType::ClosingParen))
    {
        return parameters;
    }

    do
    {
        auto parameter = ParseTypeAnnotation();
        parameters->push_back(parameter);

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
                throw ParserError(std::format(
                    "Expected ',' or ')', got token '{}' of type '{}' instead.", next.lexeme, str(next.type)
                ));
            }
        }
    } while (true);
}

std::shared_ptr<TypeExpression> Parser::ParseStruct()
{
    ExpectKeyword(Keyword::Structure);
    auto members = ParseMemberDeclarationList();
    return std::make_shared<StructExpression>(members);
}

std::shared_ptr<StructMemberDeclarationList> Parser::ParseMemberDeclarationList()
{
    auto members = std::make_shared<StructMemberDeclarationList>();
    Expect(TokenType::OpeningBrace);
    while (ConsumeAll(TokenType::Semicolon).type != TokenType::ClosingBrace)
    {
        auto member = ParseStatement();
        auto member_as_declaration = dynamic_pointer_cast<Declaration>(member);
        if (!member)
        {
            throw ParserError("Only declarations are allowed in struct declarations.");
        }
        if (!member_as_declaration->annotation)
        {
            throw ParserError("Struct member declarations require a type annotation.");
        }
        Expect(TokenType::Semicolon);
        members->push_back(member_as_declaration);
    }
    Expect(TokenType::ClosingBrace);
    return members;
}

std::shared_ptr<MemberInitializerList> Parser::ParseMemberInitializerList()
{
    Expect(TokenType::OpeningBrace);

    auto member_initializer_list = std::make_shared<MemberInitializerList>();
    while (ConsumeAll(TokenType::Semicolon).type != TokenType::ClosingBrace)
    {
        auto member = Expect(TokenType::Identifier);
        Expect(TokenType::Equals);
        auto value = ParseExpression();
        Expect(TokenType::Semicolon);

        auto member_initializer = std::make_shared<MemberInitializer>();
        member_initializer->member = std::any_cast<std::string>(member.data);
        member_initializer->value = value;

        member_initializer_list->push_back(member_initializer);
    }

    Expect(TokenType::ClosingBrace);

    return member_initializer_list;
}

std::shared_ptr<Statement> Parser::ParseAlternativeFunctionDeclaration()
{
    ExpectKeyword(Keyword::Function);

    auto identifier = Expect(TokenType::Identifier);

    auto parameters = ParseParameterDeclarationList();
    Expect(TokenType::Arrow);
    auto return_type = ParseTypeAnnotation();

    Expect(TokenType::OpeningBrace);
    auto statements = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);

    auto parameter_list_annotation = *parameters | std::views::transform([](auto param) { return param->annotation; });
    auto type_annotation = std::make_shared<FunctionTypeAnnotation>(
        std::make_shared<ParameterListAnnotation>(parameter_list_annotation.begin(), parameter_list_annotation.end()),
        return_type
    );

    auto function = std::make_shared<Function>(parameters, return_type, statements);

    return std::make_shared<Declaration>(std::any_cast<std::string>(identifier.data), type_annotation, function);
}

std::shared_ptr<Statement> Parser::ParseAlternativeStructDeclaration()
{
    ExpectKeyword(Keyword::Structure);
    auto identifier = Expect(TokenType::Identifier);

    auto members = ParseMemberDeclarationList();

    auto struct_expression = std::make_shared<StructExpression>(members);
    return std::make_shared<TypeDeclaration>(std::any_cast<std::string>(identifier.data), struct_expression);
}

std::shared_ptr<Statement> Parser::ParseAlternativeMethodDeclaration()
{
    ExpectKeyword(Keyword::Method);

    auto identifier = Expect(TokenType::Identifier);

    auto parameters = ParseParameterDeclarationList();
    Expect(TokenType::Arrow);
    auto return_type = ParseTypeAnnotation();

    Expect(TokenType::OpeningBrace);
    auto statements = ParseStatementBlock(TokenType::ClosingBrace);
    Expect(TokenType::ClosingBrace);

    auto parameter_list_annotation = *parameters | std::views::transform([](auto param) { return param->annotation; });
    auto function_annotation = std::make_shared<FunctionTypeAnnotation>(
        std::make_shared<ParameterListAnnotation>(parameter_list_annotation.begin(), parameter_list_annotation.end()),
        return_type
    );
    auto method_annotation = std::make_shared<MethodTypeAnnotation>(function_annotation);

    auto function = std::make_shared<Function>(parameters, return_type, statements);

    return std::make_shared<Declaration>(std::any_cast<std::string>(identifier.data), method_annotation, function);
}

ParserError::ParserError(std::string message)
    : message_{message}
{
}

std::string ParserError::GetMessage() const
{
    return message_;
}

}  // namespace l0
