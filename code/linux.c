
#include "shared.c"

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

typedef enum
{
#if Architecture_X64
    SyscallNumber_Write     = (1),
    SyscallNumber_Exit      = (60),
#else
#   error Linux syscall numbers are not defined for this architecture
#endif
} syscall_number;

typedef struct
{
    syscall_number Number;
    usize Arg0;
    usize Arg1;
    usize Arg2;
    usize Arg3;
    usize Arg4;
    usize Arg5;
} linux_syscall_info;

#define LinuxSyscall(...) LinuxSyscallWithInfo((linux_syscall_info){__VA_ARGS__})

local ssize LinuxSyscallWithInfo(linux_syscall_info Info)
{
    ssize Result = 0;

#if Architecture_X64
    register usize R10 __asm__("r10") = Info.Arg3;
    register usize R8  __asm__("r8")  = Info.Arg4;
    register usize R9  __asm__("r9")  = Info.Arg5;

    __asm__ volatile (
        "syscall" :
        "=a"(Result) :
        "a"(Info.Number),
        "D"(Info.Arg0),
        "S"(Info.Arg1),
        "d"(Info.Arg2),
        "r"(R10),
        "r"(R8),
        "r"(R9) :
        "memory", "rcx", "r11"
    );
#else
#   error Linux syscall is not implemented for this architecture
#endif

    return (Result);
}

local usize WriteStdOut(void* Data, usize Size)
{
    ssize Written = (ssize)LinuxSyscall(
        SyscallNumber_Write,
        STDOUT_FILENO,
        (usize)Data,
        Size
    );

    usize Result = Maximum(0, Written);
    return (Result);
}

local usize WriteStdErr(void* Data, usize Size)
{
    ssize Written = (ssize)LinuxSyscall(
        SyscallNumber_Write,
        STDERR_FILENO,
        (usize)Data,
        Size
    );

    usize Result = Maximum(0, Written);
    return (Result);
}

local void Exit(u8 ExitCode)
{
    LinuxSyscall(SyscallNumber_Exit, ExitCode);
}

void EntryPoint(void)
{
    char MessageForStdOut[] = "Hello, world from stdout!\n";
    char MessageForStdErr[] = "Hello, world from stderr!\n";

    WriteStdOut(MessageForStdOut, sizeof(MessageForStdOut) - 1);
    WriteStdErr(MessageForStdErr, sizeof(MessageForStdErr) - 1);

    Exit(0);
}

void* memset(void* DestInit, s32 Byte, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = 0;

    return (Dest);
}

