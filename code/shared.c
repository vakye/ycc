
// ===================================================================================
// NOTE(vak): Shared definitions that are commonly used throughout the codebase.
// ===================================================================================

// ===================================================================================
// NOTE(vak): This file is expected to be included by every other file in this
// codebase, so it must have 0 dependencies.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Architecture detection
//
// Detects instruction set architecture (ISA) and sets #define accordingly:
//          + x86_64  -> Architecture_x64     = 1
//          + ARM64   -> Architecture_ARM64   = 1
//          + RISCV64 -> Architecture_RISCV64 = 1
//
// If an architecture is not detected, then its corresponding #define would be
// set to 0.
// ===================================================================================

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

// ===================================================================================
// NOTE(vak): Keywords
// ===================================================================================

// NOTE(vak): Alias for "static". Marks a function or global variable as being
// visible only within this translation unit. The compiler may perform additional
// optimizations based on this fact. Almost all functions and global variables should
// be marked "local" if they aren't expected to be exported for outside (dynamic
// library or the like) usage.

#define local static

// ===================================================================================
// NOTE(vak): Macros
// ===================================================================================

// NOTE(vak):
// ArrayCount() is exclusively used for arrays that are declared with a constant size.
// For example:
//      int MyArray[40];
//      ArrayCount(MyArray) = 40
//
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

// ===================================================================================
// NOTE(vak): Integer types
// ===================================================================================

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// NOTE(vak): Largest integer type supported by an ISA. Should be the same
// size as void* type. Since we're targeting 64-bit platforms, these two
// types are defined as 64-bit integers.

typedef s64 ssize;
typedef u64 usize;

// ===================================================================================
// NOTE(vak): Boolean types
// ===================================================================================

typedef u8 b8;
typedef u32 b32;

// ===================================================================================
// NOTE(vak): Constant values
// ===================================================================================

#define true  (1)
#define false (0)

// ===================================================================================
// NOTE(vak): Memory
// ===================================================================================

// NOTE(vak): Since we don't depend on the CRT, we have to provide memset and memcpy
// as the compiler may emit calls to such functions. For example, when a large struct
// is zero-initialized, the compiler may generate a memset(&Struct, 0, sizeof(Struct)).

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

// NOTE(vak): General aliases for memory functions. These are preferred over
// memset/memcpy.

local void ZeroMemory(void* Dest, usize Size)               { memset(Dest, 0, Size); }
local void FillMemory(void* Dest, u8 Byte, usize Size)      { memset(Dest, Byte, Size); }
local void CopyMemory(void* Dest, void* Source, usize Size) { memcpy(Dest, Source, Size); }

// ===================================================================================
// NOTE(vak): Strings
// ===================================================================================

typedef struct
{
    char* Data;
    usize Size;
} string;

// NOTE(vak): Preferred way to zero-initialize a string.
// For example, instead of writing "string A = {0}", you can write
// "string A = NilString"

#define NilString (string){0}

// NOTE(vak): Convenient macro for defining a string literal.
// For example:
//      Str("Hello, world!")
// will create a 'string' struct containing a pointer to
// "Hello, world!" and the size of the string
// literal (excluding null terminator).

#define Str(Literal) (string){Literal, sizeof(Literal) - 1}

// NOTE(vak): Macro for constructing a string from a character
// buffer.
// For example:
//      char* Digits = ...;
//      usize DigitCount = ...;
//
//      string DigitString = StrData(Digits, DigitCount);

#define StrData(Data, Size) (string){Data, Size}

