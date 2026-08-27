
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
    // directly to their corresponding ASCII codes. For example, if you
    // want to detect the plus token '+', then you can write
    //          if (GetTokenKind(...) == '+')
} token_kind;

typedef u32 token_id;
#define MaxTokenCount U32Max

typedef struct
{
    string      Code;
    token_id    FirstID;
    u32         Count;
} token_array;

#define NilTokenArray (token_array){0}

local void          SetupLexer          (void);
local void          ResetLexer          (void);
local token_array   Tokenize            (string Code);

local u32           IndexFromTokenID    (token_array Tokens, token_id TokenID);
local token_id      TokenIDFromIndex    (token_array Tokens, u32 Index);

local token_kind    GetTokenKind        (token_array Tokens, u32 Index);
local string        GetTokenString      (token_array Tokens, u32 Index);
local usize         GetTokenInteger     (token_array Tokens, u32 Index); // NOTE(vak): Only use with TokenKind_Integer

local void          ErrorAtToken        (token_array Tokens, u32 Index, string Message);
local void          PrintTokens         (token_array Tokens);

// ===================================================================================
// NOTE(vak): Internal Interface
// ===================================================================================

typedef struct
{
    token_kind  Kind;
    u32         From;
    u32         Size;
} token;

typedef struct
{
    arena_id    TokenArenaID;
} lexer;

local b32           IsWhitespace        (char Character);
local b32           IsDigit             (char Character);
local b32           IsPunctuation       (char Character);

local usize         SkipWhitespace      (string Code, usize CurrentlyAt);
local token         TokenizeDigit       (string Code, usize CurrentlyAt);
local token         TokenizePunctuation (string Code, usize CurrentlyAt);

local token*        GetToken            (token_id TokenID);
local usize         GetTokenCount       (void);
local void          PushToken           (token_kind Kind, u32 From, u32 Size);

local void          ErrorAtLocation     (string Code, usize From, string Message);

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

local lexer Lexer = {0};

#define DefaultTokensCommited (16384)
#define DefaultTokensReserved (MaxTokenCount)

local void SetupLexer(void)
{
    Lexer.TokenArenaID = MakeArena(
        DefaultTokensCommited * sizeof(token),
        DefaultTokensReserved * sizeof(token) 
    );

    AlwaysAssert(!IsNilArenaID(Lexer.TokenArenaID));
}

local void ResetLexer(void)
{
    ResetArena(Lexer.TokenArenaID);
}

local token_array Tokenize(string Code)
{
    token_array Result =
    {
        .Code       = Code,
        .FirstID    = GetTokenCount(),
        .Count      = 0,
    };

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
        else ErrorAtLocation(Code, Index, Str("Unknown character in input"));

        Index += Token.Size;

        PushToken(Token.Kind, Token.From, Token.Size);
    }

    AlwaysAssert(Index == Code.Size);

    PushToken(TokenKind_EOF, Index, 0);

    Result.Count = (u32)(GetTokenCount() - Result.FirstID);

    return (Result);
}

local u32 IndexFromTokenID(token_array Tokens, token_id TokenID)
{
    AlwaysAssert(TokenID >= Tokens.FirstID);
    AlwaysAssert(TokenID <  Tokens.FirstID + Tokens.Count);

    u32 Index = TokenID - Tokens.FirstID;
    return (Index);
}

local token_id TokenIDFromIndex(token_array Tokens, u32 Index)
{
    AlwaysAssert(Index < Tokens.Count);
    token_id TokenID = Tokens.FirstID + Index;
    return (TokenID);
}

local token_kind GetTokenKind(token_array Tokens, u32 Index)
{
    token* Token = GetToken(TokenIDFromIndex(Tokens, Index));
    token_kind Kind = Token->Kind;
    return (Kind);
}

local string GetTokenString(token_array Tokens, u32 Index)
{
    token* Token = GetToken(TokenIDFromIndex(Tokens, Index));

    AlwaysAssert(Token->From + Token->Size <= Tokens.Code.Size);

    string String = StrData(Tokens.Code.Data + Token->From, Token->Size);
    return (String);
}

local usize GetTokenInteger(token_array Tokens, u32 Index)
{
    AlwaysAssert(Index < Tokens.Count);
    AlwaysAssert(GetTokenKind(Tokens, Index) == TokenKind_Integer);

    usize Result = 0;

    string Digits = GetTokenString(Tokens, Index);
    for (usize At = 0; At < Digits.Size; At++)
    {
        Result *= 10;
        Result += (Digits.Data[At] - '0');
    }

    return (Result);
}

local void ErrorAtToken(token_array Tokens, u32 Index, string Message)
{
    AlwaysAssert(Index < Tokens.Count);

    token* Token = GetToken(TokenIDFromIndex(Tokens, Index));

    ErrorAtLocation(Tokens.Code, Token->From, Message);
}

local void PrintTokens(token_array Tokens)
{
    for (u32 Index = 0; Index < Tokens.Count; Index++)
    {
        Print(StdOut, Str("    '"));
        Print(StdOut, GetTokenString(Tokens, Index));
        Print(StdOut, Str("'"));
        PrintNewLine(StdOut);
    }
}

// ===================================================================================
// NOTE(vak): Internal Implementation
// ===================================================================================

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

local token* GetToken(token_id TokenID)
{
    AlwaysAssert(TokenID < GetTokenCount());

    token* Token = (token*)GetArenaBase(Lexer.TokenArenaID) + TokenID;
    return (Token);
}

local usize GetTokenCount(void)
{
    usize Count = (GetArenaUsed(Lexer.TokenArenaID) / sizeof(token));
    AlwaysAssert(Count <= MaxTokenCount);

    return (Count);
}

local void PushToken(token_kind Kind, u32 From, u32 Size)
{
    AlwaysAssert(GetTokenCount() < MaxTokenCount);

    token* Token = PushArena(Lexer.TokenArenaID, token);

    ZeroType(Token);

    Token->Kind = Kind;
    Token->From = From;
    Token->Size = Size;
}

local void ErrorAtLocation(string Code, usize From, string Message)
{
    usize Padding = From;

    {
        Print(StdErr, Str("[\033[31mError\033[0m]: "));
        Println(StdErr, Message);
    }

    {
        Padding += Print(StdErr, Str("    '"));
        Print(StdErr, Code);
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

