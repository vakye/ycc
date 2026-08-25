
// ===================================================================================
// NOTE(vak): Lexer: responsible for converting a code string into a sequence of
// tokens.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "print.c"
#include "error.c"

// ===================================================================================
// NOTE(vak): Interface
// ===================================================================================

typedef enum
{
    // NOTE(vak): Control codes, alphabetical and digit ASCII codes are free
    // for usage in representing other token kinds (excluding EOF).

    TokenKind_EOF       = 0,
    TokenKind_Integer   = 1,

    // NOTE(vak): One-character punctuation token kinds are mapped
    // directly to their correspondin ASCII codes. For example, if you
    // want to detect the plus token '+', then you can write
    //          if (GetTokenKind(...) == '+')
} token_kind;

typedef u32 token_id;

// NOTE(vak): Each Tokenize() call will completely reset the token buffer.
// Furthermore, an EOF token (Token.Kind = TokenKind_EOF) will always be
// added at the end of the buffer. Thus, GetTokenCount() will always return
// a number equal to or larger than 1 (meaning that it includes the EOF
// token).

local void          Tokenize        (string Code);
local u32           GetTokenCount   (void);

local token_kind    GetTokenKind    (token_id TokenID);
local string        GetTokenString  (token_id TokenID);
local usize         GetTokenInteger (token_id TokenID);

local void          ErrorAtLocation (u32 From, string Message);
local void          ErrorAtToken    (token_id TokenID, string Message);

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

typedef struct
{
    token_kind  Kind;
    u32         From;
    u32         Size;
} token;

typedef struct
{
    string Code;
    u32 TokenCount;
    token Tokens[4096];
} lexer;

local lexer Lexer = {0};

local b32 IsWhitespace(char Character)
{
    b32 Result =
        (Character == ' ') ||
        (Character == '\r') ||
        (Character == '\t') ||
        (Character == '\n');

    return (Result);
}

local b32 IsDigit(char Character)
{
    b32 Result =
        (Character >= '0') &&
        (Character <= '9');

    return (Result);
}

local b32 IsPunctuation(char Character)
{
    b32 Result =
        ((Character >=  33) && (Character <=  47)) ||
        ((Character >=  58) && (Character <=  64)) ||
        ((Character >=  91) && (Character <=  96)) ||
        ((Character >= 123) && (Character <= 126));

    return (Result);
}

local usize SkipWhitespace(string Code, usize CurrentlyAt)
{
    usize NewIndex = CurrentlyAt;

    while (NewIndex < Code.Size)
    {
        char Character = Code.Data[NewIndex];

        if (!IsWhitespace(Character))
            break;

        NewIndex++;
    }

    return (NewIndex);
}

local token TokenizeDigit(string Code, usize CurrentlyAt)
{
    token Token =
    {
        .Kind = TokenKind_Integer,
        .From = CurrentlyAt,
    };

    usize NewIndex = CurrentlyAt + 1;
    while (NewIndex < Code.Size)
    {
        char Character = Code.Data[NewIndex];

        if (!IsDigit(Character))
            break;

        NewIndex++;
    }

    Token.Size = NewIndex - CurrentlyAt;

    return (Token);
}

local token TokenizePunctuation(string Code, usize CurrentlyAt)
{
    token Token =
    {
        .Kind = (token_kind)Code.Data[CurrentlyAt],
        .From = CurrentlyAt,
        .Size = 1,
    };

    return (Token);
}

local token_id AddToken(token_kind Kind, u32 From, u32 Size)
{
    if (Lexer.TokenCount == ArrayCount(Lexer.Tokens))
        Panic(Str("Token array ran out of space"));

    token_id TokenID = Lexer.TokenCount++;

    Lexer.Tokens[TokenID].Kind = Kind;
    Lexer.Tokens[TokenID].From = From;
    Lexer.Tokens[TokenID].Size = Size;

    return (TokenID);
}

local void Tokenize(string Code)
{
    Lexer.Code = Code;
    Lexer.TokenCount = 0;

    usize Index = 0;
    while (Index < Code.Size)
    {
        Index = SkipWhitespace(Code, Index);

        if (Index == Code.Size)
            break;

        token Token = {0};

        char Character = Code.Data[Index];

        if (0) {}
        else if (IsDigit(Character))        Token = TokenizeDigit(Code, Index);
        else if (IsPunctuation(Character))  Token = TokenizePunctuation(Code, Index);
        else ErrorAtLocation(Index, Str("Unknown character in input"));

        Index += Token.Size;

        AddToken(Token.Kind, Token.From, Token.Size);
    }

    if (Index > Code.Size)
        Panic(Str("Tokenizer 'Index' is larger than 'Code.Size'."));

    AddToken(TokenKind_EOF, Index, 0);
}

local token* GetToken(token_id TokenID)
{
    AlwaysAssert(TokenID < Lexer.TokenCount);

    token* Token = Lexer.Tokens + TokenID;
    return (Token);
}

local u32 GetTokenCount(void)
{
    u32 Result = Lexer.TokenCount;
    return (Result);
}

local token_kind GetTokenKind(token_id TokenID)
{
    token* Token = GetToken(TokenID);
    token_kind Kind = Token->Kind;
    return (Kind);
}

local string GetTokenString(token_id TokenID)
{
    token* Token = GetToken(TokenID);

    AlwaysAssert(Token->From + Token->Size <= Lexer.Code.Size);

    string String = StrData(Lexer.Code.Data + Token->From, Token->Size);
    return (String);
}

local usize GetTokenInteger(token_id TokenID)
{
    usize Result = 0;

    AlwaysAssert(GetTokenKind(TokenID) == TokenKind_Integer);

    string Digits = GetTokenString(TokenID);
    for (usize Index = 0; Index < Digits.Size; Index++)
    {
        Result *= 10;
        Result += (Digits.Data[Index] - '0');
    }

    return (Result);
}

local void ErrorAtLocation(u32 From, string Message)
{
    usize Padding = From;

    {
        Print(StdErr, Str("[\033[31mError\033[0m]: "));
        Println(StdErr, Message);
    }

    {
        Padding += Print(StdErr, Str("    '"));
        Print(StdErr, Lexer.Code);
        Print(StdErr, Str("'"));
        PrintNewLine(StdErr);
    }

    {
        for (usize Index = 0; Index < Padding; Index++)
            PrintCharacter(StdErr, ' ');

        PrintCharacter(StdErr, '^');
        PrintNewLine(StdErr);
    }

    Exit(1);
}

local void ErrorAtToken(token_id TokenID, string Message)
{
    token* Token = GetToken(TokenID);

    ErrorAtLocation(Token->From, Message);
}

