
#pragma once

#include "shared.c"
#include "print.c"
#include "error.c"
#include "lexer.c"

typedef ssize program_main(void);

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

local void Main(void)
{

#if !Architecture_X64
    #error Sorry, only x64 (x86_64) is implemented at the moment
#endif

    string Code = Str("  1000 +200+30   +  7-  3  + 4-2 - 2");

    Tokenize(Code);

    if (GetTokenKind(0) == TokenKind_EOF)
        ErrorAtToken(0, Str("Input string is empty"));

    // NOTE(vak): Current grammar
    //      Integer    = '0'..'9'
    //      Sum        = Integer + (('+' | '-')? + Sum)
    //      Expression = Sum

    // NOTE(vak): Register usage
    //      RAX         = Accumulator
    //      RCX         = Right hand operand
    //
    // For example: '1 + 2 + 3' will become
    //          mov rax, 1
    //          mov rcx, 2
    //          add rax, rcx
    //          mov rcx, 3
    //          add rax, rcx
    //          ret

    token_id TokenID = 0;

    // NOTE(vak): First integer goes into RAX
    {
        if (GetTokenKind(TokenID) != TokenKind_Integer)
            ErrorAtToken(0, Str("Expected an integer"));

        // NOTE(vak):
        // 48 b8 (Imm64)    mov rax, Imm64

        Emit16(0xb848);
        Emit64(GetTokenInteger(TokenID));

        TokenID++;
    }

    // NOTE(vak): Either stop parsing or parse an operation
    // along with its corresponding operand
    for (;;)
    {
        token_kind TokenKind = GetTokenKind(TokenID);

        if (TokenKind == TokenKind_EOF)
        {
            break;
        }
        else if (TokenKind == '+')
        {
            TokenID++;

            if (GetTokenKind(TokenID) != TokenKind_Integer)
                ErrorAtToken(0, Str("Expected an integer"));

            // NOTE(vak):
            // 48 b9 (Imm64)    mov rcx, Imm64
            // 48 03 c1         add rax, rcx

            Emit16(0xb948);
            Emit64(GetTokenInteger(TokenID));
            Emit24(0xc10348);

            TokenID++;
        }
        else if (TokenKind == '-')
        {
            TokenID++;

            if (GetTokenKind(TokenID) != TokenKind_Integer)
                ErrorAtToken(0, Str("Expected an integer"));

            // NOTE(vak):
            // 48 b9 (Imm64)    mov rcx, Imm64
            // 48 2b c1         sub rax, rcx

            Emit16(0xb948);
            Emit64(GetTokenInteger(TokenID));
            Emit24(0xc12b48);

            TokenID++;
        }
        else
        {
            ErrorAtToken(TokenID, Str("Syntax error"));
        }
    }

    // NOTE(vak):
    // c3   ret

    Emit8(0xc3);

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

