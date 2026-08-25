
// ===================================================================================
// NOTE(vak): Memory management functions
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "platform.c"
#include "error.c"

// ===================================================================================
// NOTE(vak): Interface
// ===================================================================================

// NOTE(vak): The preferred way to manage memory is via arena allocators. Arena
// allocators reserves a continuous region of memory, and suballocates from that
// region. Allocations happen in a continuous manner much like how a dynamic array
// would operate. Deallocations can only happen by resetting an arena, thereby
// freeing all allocations at once.

// NOTE(vak): A user can retrieve the allocation pointer by calculating
//      (u8*)GetArenaBase(ArenaID) + GetArenaUsed(ArenaID)
//
// Using PushArena*() functions will return the current allocation pointer,
// and bump up the 'Used' counter (thereby incrementing the allocation pointer).

// NOTE(vak): The arena allocator will make calls to the operating system to
// commit memory pages on demand, so you can set 'MinReserved' to a large value
// to have no worries about running out of memory while being sure that you
// aren't using too much memory (set 'MinCommited' to a reasonable value).

typedef struct
{
    u32 U32[1];
    u16 U16[2];
    u8  U8 [4];
} arena_id;

#define NilArenaID (arena_id){0}
#define IsNilArenaID(ArenaID) ((ArenaID).U32[0] == 0)

local arena_id  MakeArena       (usize MinCommited, usize MinReserved);
local void      ResetArena      (arena_id ArenaID);
local void*     GetArenaBase    (arena_id ArenaID);
local usize     GetArenaUsed    (arena_id ArenaID);
local void*     PushArenaSize   (arena_id ArenaID, usize Size);

#define PushArena(ArenaID, Type)                (Type*)PushArenaSize(ArenaID, sizeof(Type))
#define PushArenaArray(ArenaID, Type, Count)    (Type*)PushArenaSize(ArenaID, sizeof(Type) * (Count))

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

// NOTE(vak): The arena granule size is the alignment of the 'Reserved' and
// 'Commited' field of an arena allocator. Changing this will affect how
// much additional memory may be commited when it is necessary to expand.
// This value should be large enough as to prevent performing too many system
// calls, but also small enough so that we don't waste too much memory.

#define ArenaGranuleSize MB(1)

typedef struct
{
    void* Base;
    usize Used;
    usize Commited;
    usize Reserved;
} arena;

local arena Arenas[64] = {0};
local usize ArenaCount = 0;

local arena* GetArena(arena_id ArenaID)
{
    AlwaysAssert(ArenaID.U32[0] > 0);
    AlwaysAssert(ArenaID.U32[0] <= ArenaCount);

    arena* Arena = Arenas + (ArenaID.U32[0] - 1);
    return (Arena);
}

local arena_id MakeArena(usize MinCommited, usize MinReserved)
{
    AlwaysAssert(ArenaCount < ArrayCount(Arenas));

    arena_id ArenaID = {.U32[0] = 1 + ArenaCount};
    ArenaCount++;

    arena* Arena = GetArena(ArenaID);

    Arena->Commited = AlignUp(MinCommited, ArenaGranuleSize);
    Arena->Reserved = AlignUp(MinReserved, ArenaGranuleSize);

    AlwaysAssert(Arena->Reserved > 0);
    AlwaysAssert(Arena->Reserved >= Arena->Commited);

    Arena->Base = ReserveMemory(Arena->Reserved);

    AlwaysAssert(Arena->Base);

    if (Arena->Commited)
    {
        b32 CommitResult = CommitMemory(Arena->Base, Arena->Commited);
        AlwaysAssert(CommitResult);
    }

    return (ArenaID);
}

local void ResetArena(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    Arena->Used = 0;
}

local void* GetArenaBase(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    void* Result = Arena->Base;
    return (Result);
}

local usize GetArenaUsed(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    usize Result = Arena->Used;
    return (Result);
}

local void* PushArenaSize(arena_id ArenaID, usize Size)
{
    arena* Arena = GetArena(ArenaID);

    if (Arena->Used + Size > Arena->Commited)
    {
        usize ExpandSize = (Arena->Used + Size) - (Arena->Commited);

        usize CommitSize = AlignUp(ExpandSize, ArenaGranuleSize);
        void* CommitDest = (u8*)Arena->Base + Arena->Commited;

        AlwaysAssert(Arena->Commited + CommitSize <= Arena->Reserved);

        b32 CommitResult = CommitMemory(CommitDest, CommitSize);
        AlwaysAssert(CommitResult);

        Arena->Commited += CommitSize;
    }

    void* Result = (u8*)Arena->Base + Arena->Used;
    Arena->Used += Size;

    return (Result);
}

