
#pragma once

#include "shared.c"
#include "print.c"
#include "error.c"

typedef ssize program_main(void);

local void Main(void)
{

#if !Architecture_X64
    #error Sorry, only x64 (x86_64) is implemented at the moment
#endif

    string Code = Str("1337");

    u64 ReturnValue = 0;

    for (usize Index = 0; Index < Code.Size; Index++)
    {
        ReturnValue *= 10;
        ReturnValue += (Code.Data[Index] - '0');
    }

    u8 Assembly[] =
    {
        // NOTE(vak):

        //           Fill in imm64
        //                 |
        //                 v
        // 48 b8 __ __ __ __ __ __ __ __    mov rax, Imm64
        // c3                               ret

        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xc3,
    };

    CopyMemory(Assembly + 2, &ReturnValue, 8);

    void* ExecutableMemory = MapExecutableMemory(Assembly, sizeof(Assembly));

    if (!ExecutableMemory)
        Panic(Str("Failed to map executable memory"));

    program_main* ProgramMain = (program_main*)ExecutableMemory;
    ssize ProgramResult = ProgramMain();

    Print(StdOut, Str("Code string: '"));
    Print(StdOut, Code);
    Print(StdOut, Str("'"));
    PrintNewLine(StdOut);

    Print(StdOut, Str("Program result: "));
    PrintUSize(StdOut, ProgramResult);
    PrintNewLine(StdOut);
}

