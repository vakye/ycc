
#include "shared.c"
#include "platform.c"
#include "main.c"

void EntryPoint(void)
{
    Main();
    Exit(0);
}

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

