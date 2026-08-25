
// ===================================================================================
// NOTE(vak): Platform file for Linux. Contains the EntryPoint and is responsible
// for calling Main() along with implementing platform-related functions.
// ===================================================================================

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "platform.c"
#include "main.c"

// ===================================================================================
// NOTE(vak): Entry Point
// ===================================================================================

void EntryPoint(void)
{
    Main();
    Exit(0);
}

// ===================================================================================
// NOTE(vak): Linux implementation of platform.c
// ===================================================================================

// NOTE(vak): Standard file descriptor numbers

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

// NOTE(vak): Protection flags used by mmap() and mprotect() syscalls

#define PROT_NONE   (0x00)
#define PROT_READ   (0x01)
#define PROT_WRITE  (0x02)
#define PROT_EXEC   (0x04)

// NOTE(vak): Memory mapping flags used by mmap()

#define MAP_PRIVATE     (0x02)
#define MAP_ANONYMOUS   (0x20)

// NOTE(vak): Error codes

#define EINTR   (4)

// NOTE(vak): System call

typedef enum
{
#if Architecture_X64
    SyscallNumber_Write         = (1),
    SyscallNumber_MMap          = (9),
    SyscallNumber_MProtect      = (10),
    SyscallNumber_Exit          = (60),
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

// NOTE(vak): Memory

local void* ReserveMemory(usize Size)
{
    AlwaysAssert(Size > 0);

    ssize MapResult = (ssize)LinuxSyscall(
        SyscallNumber_MMap,
        0,
        Size,
        PROT_NONE,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1,
        0
    );

    void* Result = (void*)Maximum(0, MapResult);
    return (Result);
}

local b32 CommitMemory(void* Memory, usize Size)
{
    AlwaysAssert(Memory);
    AlwaysAssert(Size > 0);

    ssize ProtectResult = (ssize)LinuxSyscall(
        SyscallNumber_MProtect,
        (usize)Memory,
        Size,
        PROT_READ|PROT_WRITE
    );

    b32 Result = (ProtectResult >= 0);
    return (Result);
}

local void* MapExecutableMemory(void* Code, usize CodeSize)
{
    AlwaysAssert(Code);
    AlwaysAssert(CodeSize > 0);

    ssize MapResult = (ssize)LinuxSyscall(
        SyscallNumber_MMap,
        0,
        CodeSize,
        PROT_READ|PROT_WRITE|PROT_EXEC,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1,
        0
    );

    void* Result = 0;
    if (MapResult > 0)
    {
        Result = (void*)MapResult;
        CopyMemory(Result, Code, CodeSize);
    }

    return (Result);
}

// NOTE(vak): Input/Output

local usize LinuxWrite(s32 FileDescriptor, void* Data, usize Size)
{
    AlwaysAssert(Data);

    usize WrittenSoFar = 0;
    while (WrittenSoFar < Size)
    {
        u8* Source = (u8*)Data + WrittenSoFar;
        usize Remaining = Size - WrittenSoFar;

        ssize Written = (ssize)LinuxSyscall(
            SyscallNumber_Write,
            FileDescriptor,
            (usize)Source,
            Remaining
        );

        if (Written < 0)
        {
            // NOTE(vak): Syscall may be interrupted (EINTR), so account for that.
            ssize Error = -Written;
            if (Error != EINTR)
                break;
        }
        else
        {
            WrittenSoFar += Written;
        }
    }

    return(WrittenSoFar);
}

local usize WriteStdOut(void* Data, usize Size, ...)
{
    usize Written = LinuxWrite(STDOUT_FILENO, Data, Size);
    return (Written);
}

local usize WriteStdErr(void* Data, usize Size, ...)
{
    usize Written = LinuxWrite(STDERR_FILENO, Data, Size);
    return (Written);
}

// NOTE(vak): Process control

local void Exit(u8 ExitCode)
{
    LinuxSyscall(SyscallNumber_Exit, ExitCode);
}

