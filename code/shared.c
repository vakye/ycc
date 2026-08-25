
#pragma once

#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#   define Architecture_X64 (1)
#elif defined(__aarch64__) || defined(_M_ARM64)
#   define Architecture_ARM64 (1)
#elif defined(__riscv) && (__riscv_xlen == 64)
#   define Architecture_RISCV64 (1)
#else
#   error Unknown architecture
#endif

#if !defined(Architecture_X64)
#   define Architecture_X64 (0)
#endif

#if !defined(Architecture_ARM64)
#   define Architecture_ARM64 (0)
#endif

#if !defined(Architecture_RISCV64)
#   define Architecture_RISCV64 (0)
#endif

#define local static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef s64 ssize;
typedef u64 usize;

typedef u8 b8;
typedef u32 b32;

#define true (1)
#define false (0)

void* memset(void* DestInit, s32 Byte, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = 0;

    return (Dest);
}

void* memcpy(void* DestInit, void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;

    return (Dest);
}

local void ZeroMemory(void* Dest, usize Size)               { memset(Dest, 0, Size); }
local void FillMemory(void* Dest, u8 Byte, usize Size)      { memset(Dest, Byte, Size); }
local void CopyMemory(void* Dest, void* Source, usize Size) { memcpy(Dest, Source, Size); }

typedef struct
{
    char* Data;
    usize Size;
} string;

#define NilString           (string){0}

#define Str(Literal)        (string){Literal, sizeof(Literal) - 1}
#define StrData(Data, Size) (string){Data, Size}

