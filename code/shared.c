
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

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

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

