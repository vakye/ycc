
#pragma once

#include "print.c"

local void Main(void)
{
    Println(Str("Hello, world!"));

    Println(Str("Integer printing:"));
    Print(Str("    ")); PrintUSize(123456789); PrintNewLine();
    Print(Str("    ")); PrintUSize(1337); PrintNewLine();
    Print(Str("    ")); PrintSSize(-1234); PrintNewLine();
    Print(Str("    ")); PrintSSize(10000); PrintNewLine();
}

