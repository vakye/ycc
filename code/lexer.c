
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
#include "memory.c"

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

#define MaxTokenID (U32Max)

// NOTE(vak): Each Tokenize() call will completely reset the token buffer
// before performing tokenization. Once tokenization is finished, the
// first token will always reside at TokenID = 0, and the user can
// increment their own TokenID until hitting an EOF token.

// NOTE(vak): Example usage:
//      string Code = Str("10 + 10");
//
//      SetupLexer();
//      Tokenize(Code);
//
//      for (token_id TokenID = 0; TokenID < GetTokenCount(); TokenID++)
//              ...
//              ...

local void          SetupLexer      (void);
local void          Tokenize        (string Code);
local usize         GetTokenCount   (void);

local token_kind    GetTokenKind    (token_id TokenID);
local string        GetTokenString  (token_id TokenID);
local usize         GetTokenInteger (token_id TokenID); // NOTE(vak): Only use with TokenKind_Integer

local void          ErrorAtLocation (usize From, string Message);
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
    string      Code;
    arena_id    TokenArenaID;
} lexer;

local lexer Lexer = {0};

#define DefaultTokensCommited (16384)
#define DefaultTokensReserved (MaxTokenID)

local void SetupLexer(void)
{
    Lexer.TokenArenaID = MakeArena(
        DefaultTokensCommited * sizeof(token),
        DefaultTokensReserved * sizeof(token) 
    );

    AlwaysAssert(!IsNilArenaID(Lexer.TokenArenaID));
}

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

local void AddToken(token_kind Kind, u32 From, u32 Size)
{
    AlwaysAssert(GetTokenCount() < MaxTokenID);

    token* Token = PushArena(Lexer.TokenArenaID, token);

    Token->Kind = Kind;
    Token->From = From;
    Token->Size = Size;
}

local void Tokenize(string Code)
{
    Lexer.Code = Code;

    ResetArena(Lexer.TokenArenaID);

    usize Index = 0;
    while (Index < Code.Size)
    {
        Index = SkipWhitespace(Code, Index);

        if (Index >= Code.Size)
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

    AlwaysAssert(Index == Code.Size);

    AddToken(TokenKind_EOF, Index, 0);
}

local token* GetToken(token_id TokenID)
{
    AlwaysAssert(TokenID < GetTokenCount());

    token* Token = (token*)GetArenaBase(Lexer.TokenArenaID) + TokenID;
    return (Token);
}

local usize GetTokenCount(void)
{
    usize Result = (GetArenaUsed(Lexer.TokenArenaID) / sizeof(token));

    AlwaysAssert(Result <= MaxTokenID);

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

local void ErrorAtLocation(usize From, string Message)
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

