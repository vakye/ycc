
// ===================================================================================
// NOTE(vak): Generator: Responsible for emitting machine code instructions to
// perform the operations specified by a syntax tree.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "error.c"
#include "memory.c"
#include "parser.c"

// ===================================================================================
// NOTE(vak): Interface
// ===================================================================================

typedef struct
{
    void* Code;
    usize CodeSize;
} gen_block;

local void SetupGenerator(void);
local void ResetGenerator(void);

local gen_block x64_Generate(node_id RootNode);

// ===================================================================================
// NOTE(vak): Internal Interface
// ===================================================================================

typedef struct
{
    arena_id CodeArenaID;
} generator;

local void EmitBytes    (void* Bytes, usize Size);
local void Emit8        (u8  Value);
local void Emit16       (u16 Value);
local void Emit24       (u32 Value);
local void Emit32       (u32 Value);
local void Emit40       (u64 Value);
local void Emit48       (u64 Value);
local void Emit56       (u64 Value);
local void Emit64       (u64 Value);

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

local generator Generator = {0};

#define DefaultCodeCommited (KB(256))
#define DefaultCodeReserved (GB(64))

local void SetupGenerator(void)
{
    Generator.CodeArenaID = MakeArena(
        DefaultCodeCommited,
        DefaultCodeReserved
    );

    AlwaysAssert(!IsNilArenaID(Generator.CodeArenaID));
}

local void ResetGenerator(void)
{
    ResetArena(Generator.CodeArenaID);
}

#include "generator_x64.c"

// ===================================================================================
// NOTE(vak): Internal Implementation
// ===================================================================================

local void EmitBytes(void* Bytes, usize Size)
{
    void* WriteAt = PushArenaSize(Generator.CodeArenaID, Size);

    CopyMemory(WriteAt, Bytes, Size);
}

local void Emit8 (u8  Value) { EmitBytes(&Value, 1); }
local void Emit16(u16 Value) { EmitBytes(&Value, 2); }
local void Emit24(u32 Value) { EmitBytes(&Value, 3); }
local void Emit32(u32 Value) { EmitBytes(&Value, 4); }
local void Emit40(u64 Value) { EmitBytes(&Value, 5); }
local void Emit48(u64 Value) { EmitBytes(&Value, 6); }
local void Emit56(u64 Value) { EmitBytes(&Value, 7); }
local void Emit64(u64 Value) { EmitBytes(&Value, 8); }

