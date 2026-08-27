
// ===================================================================================
// NOTE(vak): Parser: responsible for converting tokens into a syntax tree composed
// of nodes representing operations or values.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "memory.c"
#include "lexer.c"

// ===================================================================================
// NOTE(vak): Interface
// ===================================================================================

typedef enum
{
    NodeKind_Nil        = 0,

    NodeKind_Integer,   // NOTE(vak): Uses integer_node

    // NOTE(vak): Uses binary_node

    NodeKind_Add,
    NodeKind_Sub,

    NodeKind_COUNT,
} node_kind;

// NOTE(vak): node_id starts from 1, and the 0 slot is reserved for
// representing a nil node.

typedef u32 node_id;

#define NilNodeID (0)
#define MaxNodeID (U32Max - 1)

#define IsNilNodeID(NodeID) ((NodeID) == NilNodeID)

typedef struct
{
    usize Value;
} integer_node;

typedef struct
{
    node_id Left;
    node_id Right;
} binary_node;

typedef struct
{
    integer_node Integer;
    binary_node Binary;
} node_data;

local void      SetupParser     (void);
local node_id   Parse           (void);
local node_id   ParseExpression (void);
local node_id   ParseSum        (void);
local node_id   ParsePrimary    (void);

local node_kind GetNodeKind     (node_id NodeID);
local token_id  GetNodeTokenID  (node_id NodeID);
local node_data GetNodeData     (node_id NodeID);

// ===================================================================================
// NOTE(vak): Internal interface
// ===================================================================================

local token_id  ParserCurrent       (void);
local void      ParserNext          (void);
local b32       ParserMatch         (token_kind TokenKind);
local b32       ParserNextIfMatch   (token_kind TokenKind);
local void      ParserExpect        (token_kind TokenKind, string ErrorMessage);
local void      ParserExpectAndSkip (token_kind TokenKind, string ErrorMessage);

local node_id   MakeNode            (node_kind Kind, token_id TokenID);
local node_id   MakeIntegerNode     (token_id TokenID);
local node_id   MakeBinaryNode      (node_kind Kind, token_id TokenID, node_id Left, node_id Right);

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

typedef struct
{
    node_kind Kind;
    token_id  TokenID;
    node_data Data;
} node;

typedef struct
{
    arena_id NodeArenaID;
    token_id TokenID;
} parser;

local parser Parser = {0};

#define DefaultNodesCommited (16384)
#define DefaultNodesReserved (MaxNodeID + 1)

local void SetupParser(void)
{
    Parser.NodeArenaID = MakeArena(
        DefaultNodesCommited * sizeof(node),
        DefaultNodesReserved * sizeof(node)
    );

    AlwaysAssert(!IsNilArenaID(Parser.NodeArenaID));
}

local node_id Parse(void)
{
    node_id NodeID = ParseExpression();
    return (NodeID);
}

local node_id ParseExpression(void)
{
    node_id NodeID = ParseSum();
    return (NodeID);
}

local node_id ParseSum(void)
{
    node_id NodeID = ParsePrimary();

    for (;;)
    {
        token_id TokenID = ParserCurrent();

        if (ParserNextIfMatch('+'))
        {
            NodeID = MakeBinaryNode(NodeKind_Add, TokenID, NodeID, ParsePrimary());
        }
        else if (ParserNextIfMatch('-'))
        {
            NodeID = MakeBinaryNode(NodeKind_Sub, TokenID, NodeID, ParsePrimary());
        }
        else
        {
            break;
        }
    }

    return (NodeID);
}

local node_id ParsePrimary(void)
{
    node_id NodeID = NilNodeID;
    token_id TokenID = ParserCurrent();

    if (ParserNextIfMatch(TokenKind_Integer))
    {
        NodeID = MakeIntegerNode(TokenID);
    }
    else if (ParserNextIfMatch('('))
    {
        NodeID = ParseExpression();
        ParserExpectAndSkip(')', Str("Missing matching ')' in expression"));
    }
    else
    {
        ErrorAtToken(TokenID, Str("Syntax error"));
    }

    return (NodeID);
}

local usize GetNodeCount(void)
{
    usize Result = GetArenaUsed(Parser.NodeArenaID) / sizeof(node);
    return (Result);
}

local node* GetNode(node_id NodeID)
{
    AlwaysAssert(NodeID > 0);
    AlwaysAssert(NodeID <= GetNodeCount());

    node* Node = (node*)GetArenaBase(Parser.NodeArenaID) + (NodeID - 1);
    return (Node);
}

local node_kind GetNodeKind(node_id NodeID)
{
    node* Node = GetNode(NodeID);
    node_kind Result = Node->Kind;
    return (Result);
}

local token_id GetNodeTokenID(node_id NodeID)
{
    node* Node = GetNode(NodeID);
    token_id Result = Node->TokenID;
    return (Result);
}

local node_data GetNodeData(node_id NodeID)
{
    node* Node = GetNode(NodeID);
    node_data Result = Node->Data;
    return (Result);
}

local token_id ParserCurrent(void)
{
    token_id TokenID = Parser.TokenID;
    return (TokenID);
}

local void ParserNext(void)
{
    if (Parser.TokenID < GetTokenCount())
        Parser.TokenID++;
}

local b32 ParserMatch(token_kind TokenKind)
{
    b32 Result = (GetTokenKind(ParserCurrent()) == TokenKind);
    return (Result);
}

local b32 ParserNextIfMatch(token_kind TokenKind)
{
    b32 Result = ParserMatch(TokenKind);
    if (Result)
        ParserNext();

    return (Result);
}

local void ParserExpect(token_kind TokenKind, string ErrorMessage)
{
    if (!ParserMatch(TokenKind))
        ErrorAtToken(ParserCurrent(), ErrorMessage);
}

local void ParserExpectAndSkip(token_kind TokenKind, string ErrorMessage)
{
    if (!ParserNextIfMatch(TokenKind))
        ErrorAtToken(ParserCurrent(), ErrorMessage);
}

local node_id MakeNode(node_kind Kind, token_id TokenID)
{
    AlwaysAssert(GetNodeCount() < MaxNodeID);

    node_id NodeID = 1 + GetNodeCount();
    PushArena(Parser.NodeArenaID, node);

    node* Node = GetNode(NodeID);
    ZeroType(Node);

    Node->Kind      = Kind;
    Node->TokenID   = TokenID;

    return (NodeID);
}

local node_id MakeIntegerNode(token_id TokenID)
{
    node_id NodeID = MakeNode(NodeKind_Integer, TokenID);
    node* Node = GetNode(NodeID);

    Node->Data.Integer.Value = GetTokenInteger(TokenID);

    return (NodeID);
}

local node_id MakeBinaryNode(node_kind Kind, token_id TokenID, node_id Left, node_id Right)
{
    node_id NodeID = MakeNode(Kind, TokenID);
    node* Node = GetNode(NodeID);

    Node->Data.Binary.Left = Left;
    Node->Data.Binary.Right = Right;

    return (NodeID);
}

