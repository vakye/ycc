
#pragma once

#include "print.c"

typedef ssize program_main(void);

local void Main(void)
{

#if !Architecture_X64
    #error Sorry, only x64 (x86_64) is implemented at the moment
#endif

    u8 Assembly[] =
    {
        // NOTE(vak):
        // 48 c7 c0 10 00 00 00     mov rax, 16
        // c3                       ret

        0x48, 0xc7, 0xc0, 0x10, 0x00, 0x00, 0x00,
        0xc3,
    };

    void* ExecutableMemory = MapExecutableMemory(Assembly, sizeof(Assembly));

    if (!ExecutableMemory)
    {
        Println(StdErr, Str("Failed to map executable memory"));
        Exit(1);
    }

    program_main* ProgramMain = (program_main*)ExecutableMemory;
    ssize ProgramResult = ProgramMain();

    Print(StdOut, Str("Program result: "));
    PrintUSize(StdOut, ProgramResult);
    PrintNewLine(StdOut);
}

