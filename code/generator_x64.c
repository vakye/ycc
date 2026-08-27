
// ===================================================================================
// NOTE(vak): x64 Generator: Generates x86_64 instructions. The function
// x64_Generate() is implemented here.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

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

typedef u8 x64_acc_id;

// NOTE(vak): RAX, RSI, RDI, R8, R9, R10, R11
#define x64_MaxAccumulatorCount (7)

typedef struct
{
    b32 AccumulatorTaken[x64_MaxAccumulatorCount];
} x64_state;

local x64_state x64 = {0};

local x64_acc_id x64_AcquireAccumulator(void)
{
    x64_acc_id AccID = x64_MaxAccumulatorCount;

    for (x64_acc_id ScanID = 0; ScanID < x64_MaxAccumulatorCount; ScanID++)
    {
        if (!x64.AccumulatorTaken[ScanID])
        {
            AccID = ScanID;
            break;
        }
    }

    if (AccID == x64_MaxAccumulatorCount)
        Panic(Str("Code gen ran out of accumulators"));

    x64.AccumulatorTaken[AccID] = true;

    return (AccID);
}

local void x64_ReleaseAccumulator(x64_acc_id AccID)
{
    AlwaysAssert(AccID < x64_MaxAccumulatorCount);
    AlwaysAssert(x64.AccumulatorTaken[AccID] == true);

    x64.AccumulatorTaken[AccID] = false;
}

local x64_reg x64_GetAccumulatorReg(x64_acc_id AccID)
{
    AlwaysAssert(AccID < x64_MaxAccumulatorCount);

    persist x64_reg Mapping[x64_MaxAccumulatorCount] =
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
    x64_OpKind_Nil = 0,
    x64_OpKind_Imm,        // NOTE(vak): Integer immediate
    x64_OpKind_Acc,        // NOTE(vak): Accumulator
} x64_op_kind;

typedef struct
{
    x64_op_kind Kind;
    union
    {
        u64     Imm;
        x64_acc_id  AccID;
    };
} x64_op;

local x64_op x64_GenerateNode(node_id NodeID)
{
    x64_op ResultOp = {0};

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
            ResultOp.Kind = x64_OpKind_Imm;
            ResultOp.Imm  = Data.Integer.Value;
        } break;

        case NodeKind_Add:
        case NodeKind_Sub:
        {
            x64_op LeftOp = x64_GenerateNode(Data.Binary.Left);
            x64_op RightOp = x64_GenerateNode(Data.Binary.Right);

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

            if (LeftOp.Kind == x64_OpKind_Imm)
            {
                usize Imm = LeftOp.Imm;

                LeftOp.Kind = x64_OpKind_Acc;
                LeftOp.AccID = x64_AcquireAccumulator();

                x64_reg DestReg = x64_GetAccumulatorReg(LeftOp.AccID);

                u16 Instruction = 0xb848;

                Instruction += (DestReg & 0x8) >> 3; // NOTE(vak): REX.B
                Instruction += (DestReg & 0x7) << 8; // NOTE(vak): b8 + Reg

                // NOTE(vak):
                // (REX) (b8 + Reg) (Imm64)     mov Reg, Imm64
                Emit16(Instruction);
                Emit64(Imm);
            }

            // NOTE(vak): Ensure that the operands are in the correct form.

            AlwaysAssert(LeftOp.Kind == x64_OpKind_Acc);

            // NOTE(vak): Move right hand side into RCX

            if (RightOp.Kind == x64_OpKind_Imm)
            {
                // NOTE(vak):
                // 48 b9 (Imm64)    mov rcx, Imm64
                Emit16(0xb948);
                Emit64(RightOp.Imm);
            }
            else if (RightOp.Kind == x64_OpKind_Acc)
            {
                // NOTE(vak):
                // (REX) 8b (ModRM) mov rcx, Reg

                x64_reg SourceReg = x64_GetAccumulatorReg(RightOp.AccID);

                u32 Instruction = 0xc88b48;

                Instruction += (SourceReg & 0x8) >> 3;  // NOTE(vak): REX.B
                Instruction += (SourceReg & 0x7) << 16; // NOTE(vak): ModRM.rm

                Emit24(Instruction);

                // NOTE(vak): Accumulators store temporary results of nested expressions,
                // so once we use them here, we can release them for other nested expressions
                // that will be generated later.

                x64_ReleaseAccumulator(RightOp.AccID);
            }
            else
            {
                Panic(Str("Unknown RightOp.Kind not handled in code gen"));
            }

            // NOTE(vak): Peform the operation

            x64_reg DestReg = x64_GetAccumulatorReg(LeftOp.AccID);

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

            ResultOp.Kind = x64_OpKind_Acc;
            ResultOp.AccID = LeftOp.AccID;
        } break;
    }

    return (ResultOp);
}

// NOTE(vak): Prepares a stack frame for a function
local void x64_GeneratePrologue(void)
{
    // NOTE(vak):
    // 55           push rbp
    // 48 8b ec     mov rbp, rsp

    Emit32(0xec8b4855);
}

// NOTE(vak): Cleans up stack frame and return
local void x64_GenerateEpilogue(x64_op ReturnOp)
{
    if (ReturnOp.Kind == x64_OpKind_Imm)
    {
        // NOTE(vak):
        // 48 b8 Imm64      mov rax, Imm64
        Emit16(0xb848);
        Emit64(ReturnOp.Imm);
    }
    else if (ReturnOp.Kind == x64_OpKind_Acc)
    {
        // (REX) 8b (ModRM) mov rax, Reg

        x64_reg SourceReg = x64_GetAccumulatorReg(ReturnOp.AccID);

        u32 Instruction = 0xc08b48;

        Instruction += (SourceReg & 0x8) >> 3;  // NOTE(vak): REX.B
        Instruction += (SourceReg & 0x7) << 16; // NOTE(vak): ModRM.rm

        Emit24(Instruction);

        // NOTE(vak): Not necessary, but it is good to be polite
        x64_ReleaseAccumulator(ReturnOp.AccID);         
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

local gen_block x64_Generate(node_id RootNode)
{
    usize Begin = GetArenaUsed(Generator.CodeArenaID);

    x64_GeneratePrologue();
    x64_op ReturnOp = x64_GenerateNode(RootNode);
    x64_GenerateEpilogue(ReturnOp);

    usize End = GetArenaUsed(Generator.CodeArenaID);

    gen_block Block =
    {
        .Code = (u8*)GetArenaBase(Generator.CodeArenaID) + Begin,
        .CodeSize = End - Begin,
    };

    return (Block);
}

