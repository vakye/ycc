
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

// NOTE(vak): The code generator currently supports very primitive
// clobbering. There are a number of "accumulator" registers where
// results can be stored in. The accumulator registers are registers
// that are free for usage as defined by the SystemV ABI.

// NOTE(vak): There are a number of constraints that the code generator
// follows:
//      + The following registers can be used as accumulators: RAX, RSI,
//      + RDI, R8, R9, R10, R11.
//      + RCX is always reserved for right-hand side operands. In order to perform
//        an operation, the code generator always loads the right-hand side
//        into RCX regardless of whether it is necessary or not.
//      + RDX is always reserved since it is needed for div/mul instruction

// NOTE(vak): Currently, there is no support for stack clobbering, so
// when the accumulator registers run out, the code generator prints out
// an error and exits.

typedef u8 x64_reg;
enum
{
    x64_RAX = 0,
    x64_RCX = 1,
    x64_RDX = 2,
    x64_RBX = 3,
    x64_RSP = 4,
    x64_RBP = 5,
    x64_RSI = 6,
    x64_RDI = 7,
    x64_R8  = 8,
    x64_R9  = 9,
    x64_R10 = 10,
    x64_R11 = 11,
    x64_R12 = 12,
    x64_R13 = 13,
    x64_R14 = 14,
    x64_R15 = 15,

    x64_RegCount,
    x64_InvalidReg = x64_RegCount,
};

typedef u8 acc_id;

// NOTE(vak): RAX, RSI, RDI, R8, R9, R10, R11
#define MaxAccumulatorCount (7)

local b32 AccumulatorTaken[MaxAccumulatorCount] = {0};

local acc_id AcquireAccumulator(void)
{
    acc_id AccID = MaxAccumulatorCount;

    for (acc_id ScanID = 0; ScanID < MaxAccumulatorCount; ScanID++)
    {
        if (!AccumulatorTaken[ScanID])
        {
            AccID = ScanID;
            break;
        }
    }

    if (AccID == MaxAccumulatorCount)
        Panic(Str("Code gen ran out of accumulators"));

    AccumulatorTaken[AccID] = true;

    return (AccID);
}

local void ReleaseAccumulator(acc_id AccID)
{
    AlwaysAssert(AccID < MaxAccumulatorCount);
    AlwaysAssert(AccumulatorTaken[AccID] == true);

    AccumulatorTaken[AccID] = false;
}

local x64_reg GetAccumulatorReg(acc_id AccID)
{
    AlwaysAssert(AccID < MaxAccumulatorCount);

    persist x64_reg Mapping[MaxAccumulatorCount] =
    {
        x64_RAX,
        x64_RSI,
        x64_RDI,
        x64_R8,
        x64_R9,
        x64_R10,
        x64_R11,
    };

    x64_reg Reg = Mapping[AccID];
    return (Reg);
}

typedef enum
{
    OperandKind_Nil = 0,
    OperandKind_Imm,        // NOTE(vak): Integer immediate
    OperandKind_Acc,        // NOTE(vak): Accumulator
} operand_kind;

typedef struct
{
    operand_kind Kind;
    union
    {
        u64     Imm;
        acc_id  AccID;
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
        } break;

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

            // NOTE(vak): Operand form when performing operations
            //      + AccID: Left hand side
            //      + RCX: Right hand side
            //
            // The accumulator is associated with an AccID, which are mapped
            // to specific registers. Right-hand side operand is always moved
            // into RCX whenever performing a binary operation.

            // NOTE(vak): Left hand side is always either an operation or an
            // immediate. If the left hand side is an operation, then it will
            // be an accumulator. Else if it is an immediate then that means
            // we're within a nested expression, so we need a new accumulator
            // to store the results.

            if (LeftOp.Kind == OperandKind_Imm)
            {
                usize Imm = LeftOp.Imm;

                LeftOp.Kind = OperandKind_Acc;
                LeftOp.AccID = AcquireAccumulator();

                x64_reg DestReg = GetAccumulatorReg(LeftOp.AccID);

                u16 Instruction = 0xb848;

                Instruction += (DestReg & 0x8) >> 3; // NOTE(vak): REX.B
                Instruction += (DestReg & 0x7) << 8; // NOTE(vak): b8 + Reg

                // NOTE(vak):
                // (REX) (b8 + Reg) (Imm64)     mov Reg, Imm64
                Emit16(Instruction);
                Emit64(Imm);
            }

            // NOTE(vak): Ensure that the operands are in the correct form.

            AlwaysAssert(LeftOp.Kind == OperandKind_Acc);

            // NOTE(vak): Move right hand side into RCX

            if (RightOp.Kind == OperandKind_Imm)
            {
                // NOTE(vak):
                // 48 b9 (Imm64)    mov rcx, Imm64
                Emit16(0xb948);
                Emit64(RightOp.Imm);
            }
            else if (RightOp.Kind == OperandKind_Acc)
            {
                // NOTE(vak):
                // (REX) 8b (ModRM) mov rcx, Reg

                x64_reg SourceReg = GetAccumulatorReg(RightOp.AccID);

                u32 Instruction = 0xc88b48;

                Instruction += (SourceReg & 0x8) >> 3;  // NOTE(vak): REX.B
                Instruction += (SourceReg & 0x7) << 16; // NOTE(vak): ModRM.rm

                Emit24(Instruction);

                // NOTE(vak): Accumulators store temporary results of nested expressions,
                // so once we use them here, we can release them for other nested expressions
                // that will be generated later.

                ReleaseAccumulator(RightOp.AccID);
            }
            else
            {
                Panic(Str("Unknown RightOp.Kind not handled in code gen"));
            }

            // NOTE(vak): Peform the operation

            x64_reg DestReg = GetAccumulatorReg(LeftOp.AccID);

            u8 REX   = 0x48;
            u8 ModRM = 0xc1;

            REX   += (DestReg & 0x8) >> 1;  // NOTE(vak): REX.R
            ModRM += (DestReg & 0x7) << 3;  // NOTE(vak): ModRM.reg

            #define REX_OpCode_ModRM(R, O, M) \
                (R) | (O << 8) | (M << 16)

            switch (Kind)
            {
                case NodeKind_Add: Emit24(REX_OpCode_ModRM(REX, 0x03, ModRM)); break; // NOTE(vak): add Reg, rcx
                case NodeKind_Sub: Emit24(REX_OpCode_ModRM(REX, 0x2b, ModRM)); break; // NOTE(vak): sub Reg, rcx
            }

            #undef REX_OpCode_ModRM

            // NOTE(vak): Result in left-hand accumulator

            ResultOp.Kind = OperandKind_Acc;
            ResultOp.AccID = LeftOp.AccID;
        } break;
    }

    return (ResultOp);
}

// NOTE(vak): Prepares a stack frame for a function
local void GeneratePrologue(void)
{
    // NOTE(vak):
    // 55           push rbp
    // 48 8b ec     mov rbp, rsp

    Emit32(0xec8b4855);
}

// NOTE(vak): Cleans up stack frame and return
local void GenerateEpilogue(operand ReturnOp)
{
    if (ReturnOp.Kind == OperandKind_Imm)
    {
        // NOTE(vak):
        // 48 b8 Imm64      mov rax, Imm64
        Emit16(0xb848);
        Emit64(ReturnOp.Imm);
    }
    else if (ReturnOp.Kind == OperandKind_Acc)
    {
        // (REX) 8b (ModRM) mov rax, Reg

        x64_reg SourceReg = GetAccumulatorReg(ReturnOp.AccID);

        u32 Instruction = 0xc08b48;

        Instruction += (SourceReg & 0x8) >> 3;  // NOTE(vak): REX.B
        Instruction += (SourceReg & 0x7) << 16; // NOTE(vak): ModRM.rm

        Emit24(Instruction);

        // NOTE(vak): Not necessary, but it is good to be polite
        ReleaseAccumulator(ReturnOp.AccID);         
    }
    else
    {
        Panic(Str("Unknown ReturnOp.Kind not handled in code gen"));
    }

    // NOTE(vak):
    // 48 8b e5     mov rsp, rbp
    // 5d           pop rbp
    // c3           ret

    Emit40(0xc35de58b48);
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

    string Code = Str("((  (1000 + 400) - (200+200)) +(200+30))   +  (7-  (3  + (4-2) - 2))");

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
    operand ReturnOp = GenerateNode(RootNode);
    GenerateEpilogue(ReturnOp);

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

