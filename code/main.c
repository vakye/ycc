
// ===================================================================================
// NOTE(vak): Main program logic. Contains the Main() function.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "print.c"
#include "error.c"
#include "lexer.c"
#include "parser.c"

// ===================================================================================
// NOTE(vak): Printing functions
// ===================================================================================

local void PrintTokens(void)
{
    usize TokenCount = GetTokenCount();
    for (token_id TokenID = 0; TokenID < TokenCount; TokenID++)
    {
        Print(StdOut, Str("    '"));
        Print(StdOut, GetTokenString(TokenID));
        Print(StdOut, Str("'"));
        PrintNewLine(StdOut);
    }
}

local string GetNodeKindString(node_kind Kind)
{
    persist string KindStrings[NodeKind_COUNT] =
    {
        [NodeKind_Nil]          = StaticStr("Nil"),
        [NodeKind_Integer]      = StaticStr("Integer"),
        [NodeKind_Add]          = StaticStr("Add"),
        [NodeKind_Sub]          = StaticStr("Sub"),
    };

    AlwaysAssert(Kind < NodeKind_COUNT);

    string Result = KindStrings[Kind];
    return (Result);
}

local void PrintNode(node_id NodeID)
{
    if (IsNilNodeID(NodeID))
        return;

    persist usize Depth = 0;

    Depth++;

    node_kind Kind = GetNodeKind(NodeID);
    node_data Data = GetNodeData(NodeID);

    for (usize Index = 0; Index < Depth; Index++)
        Print(StdOut, Str("    "));

    Print(StdOut, GetNodeKindString(Kind));
    Print(StdOut, Str(": "));

    switch (Kind)
    {
        default: {} break;

        case NodeKind_Integer:
        {
            PrintUSize(StdOut, Data.Integer.Value);
            PrintNewLine(StdOut);
        } break;

        case NodeKind_Add:
        case NodeKind_Sub:
        {
            PrintNewLine(StdOut);
            PrintNode(Data.Binary.Left);
            PrintNode(Data.Binary.Right);
        } break;
    }

    Depth--;
}

// ===================================================================================
// NOTE(vak): Machine code generation
// ===================================================================================

local u8    MachineCode[KB(64)] = {0};
local usize MachineCodeSize     = 0;

local void EmitBytes(void* Bytes, usize Size)
{
    if (MachineCodeSize + Size > sizeof(MachineCode))
        Panic(Str("Machine code buffer ran out of space"));

    CopyMemory(MachineCode + MachineCodeSize, Bytes, Size);

    MachineCodeSize += Size;
}

local void Emit8 (u8  Value) { EmitBytes(&Value, 1); }
local void Emit16(u16 Value) { EmitBytes(&Value, 2); }
local void Emit24(u32 Value) { EmitBytes(&Value, 3); }
local void Emit32(u32 Value) { EmitBytes(&Value, 4); }
local void Emit40(u64 Value) { EmitBytes(&Value, 5); }
local void Emit48(u64 Value) { EmitBytes(&Value, 6); }
local void Emit56(u64 Value) { EmitBytes(&Value, 7); }
local void Emit64(u64 Value) { EmitBytes(&Value, 8); }

// NOTE(vak): Prepares a stack frame for a function
local void GeneratePrologue(void)
{
    // NOTE(vak):
    // 55           push rbp
    // 48 8b ec     mov rbp, rsp

    Emit32(0xec8b4855);
}

// NOTE(vak): Cleans up stack frame and return
local void GenerateEpilogue(void)
{
    // NOTE(vak):
    // 48 8b e5     mov rsp, rbp
    // 5d           pop rbp
    // c3           ret

    Emit40(0xc35de58b48);
}

// NOTE(vak): The code generator is very primitive right now.
// All results and left-hand side goes into the accumulator (RAX),
// and all right-hand side operands goes into (RCX). This means
// that it can only handle one expression at a time, and isn't able to
// clobber registers to evaluate nested expressions.

typedef enum
{
    OperandKind_Nil = 0,
    OperandKind_Imm,        // NOTE(vak): Integer immediate
    OperandKind_Acc,        // NOTE(vak): Accumulator: RAX
} operand_kind;

typedef struct
{
    operand_kind Kind;
    union
    {
        u64 Imm;
    };
} operand;

local operand GenerateNode(node_id NodeID)
{
    operand ResultOp = {0};

    if (IsNilNodeID(NodeID))
        return (ResultOp);

    node_kind Kind = GetNodeKind(NodeID);
    node_data Data = GetNodeData(NodeID);

    switch (Kind)
    {
        default:
        {
            Print(StdErr, Str("unimplemented node kind '"));
            Print(StdErr, GetNodeKindString(Kind));
            Print(StdErr, Str("'"));
            PrintNewLine(StdErr);
            Exit(1);
        };

        case NodeKind_Integer:
        {
            ResultOp.Kind = OperandKind_Imm;
            ResultOp.Imm  = Data.Integer.Value;
        } break;

        case NodeKind_Add:
        case NodeKind_Sub:
        {
            operand LeftOp = GenerateNode(Data.Binary.Left);
            operand RightOp = GenerateNode(Data.Binary.Right);

            // NOTE(vak): Register usage when performing operations
            //      + RAX: Left hand side
            //      + RCX: Right hand side
            //
            // We treat RAX as the accumulator, so all results end up in RAX.

            if (LeftOp.Kind == OperandKind_Imm)
            {
                // NOTE(vak):
                // 48 b8 (Imm64)    mov rax, Imm64
                Emit16(0xb848);
                Emit64(LeftOp.Imm);

                LeftOp.Kind = OperandKind_Acc;
            }

            // NOTE(vak): Ensure that the operands are in the correct form.

            // Currently, we only expect integers for the right hand side
            // since the parser doesn't support operators of different precedence
            // nor does it support parentheses.

            AlwaysAssert(LeftOp.Kind == OperandKind_Acc);
            AlwaysAssert(RightOp.Kind == OperandKind_Imm);

            // NOTE(vak):
            // 48 b9 (Imm64)    mov rcx, Imm64
            Emit16(0xb948);
            Emit64(RightOp.Imm);

            // NOTE(vak): Peform the operation

            switch (Kind)
            {
                case NodeKind_Add: Emit24(0xc10348); break; // NOTE(vak): 48 03 c1 add rax, rcx
                case NodeKind_Sub: Emit24(0xc12b48); break; // NOTE(vak): 48 2b c1 sub rac, rcx
            }

            // NOTE(vak): Result in RAX (accumulator)

            ResultOp.Kind = OperandKind_Acc;
        } break;
    }

    return (ResultOp);
}

// ===================================================================================
// NOTE(vak): Main function
// ===================================================================================

typedef ssize program_main(void);

local void Main(void)
{

#if !Architecture_X64
    #error Sorry, only x64 (x86_64) is implemented at the moment
#endif

    string Code = Str("  1000 +200+30   +  7-  3  + 4-2 - 2");

    SetupLexer();
    SetupParser();

    Tokenize(Code);

    {
        Println(StdOut, Str("Tokenizer output:"));
        PrintTokens();
    }

    node_id RootNode = Parse();

    {
        Println(StdOut, Str("Parser output:"));
        PrintNode(RootNode);
    }

    GeneratePrologue();
    GenerateNode(RootNode);
    GenerateEpilogue();

    void* ExecutableMemory = MapExecutableMemory(MachineCode, MachineCodeSize);

    if (!ExecutableMemory)
        Panic(Str("Failed to map executable memory"));

    program_main* ProgramMain = (program_main*)ExecutableMemory;
    ssize ProgramResult = ProgramMain();

    Print(StdOut, Str("Code string: '"));
    Print(StdOut, Code);
    Print(StdOut, Str("'"));
    PrintNewLine(StdOut);

    Print(StdOut, Str("Program result: "));
    PrintUSize(StdOut, ProgramResult);
    PrintNewLine(StdOut);
}

