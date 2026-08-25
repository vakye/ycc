
#pragma once

#include "shared.c"
#include "platform.c"
#include "print.c"

#define AlwaysAssert(Expression) \
    if (!(Expression)) Panic(Str("Assertion '" #Expression "' has failed."))

#define Panic(Message) PanicFull(Str(__FILE__), __LINE__, Message)

local void PanicFull(string File, usize Line, string Message)
{
    Print(StdErr, File);
    PrintCharacter(StdErr, ':');
    PrintUSize(StdErr, Line);
    PrintCharacter(StdErr, ':');
    Print(StdErr, Str(" [Fatal Error]: "));
    Println(StdErr, Message);
    Exit(1);
}

