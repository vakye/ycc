
// ===================================================================================
// NOTE(vak): Printing functions for characters, strings, integers, ...
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "platform.c"

// ===================================================================================
// NOTE(vak): Interface
//
// All print functions must receive a 'print_out' struct so that they know where to
// write to. Two standard 'print_out' structs are provided that are 'StdOut' and
// 'StdErr'.
//
// Furthermore, all print functions will return the number of bytes written. A print
// function may write less bytes than requested.
//
// ===================================================================================

typedef usize print_write(void* Data, usize Size, void* UserData);

typedef struct
{
    print_write*    Write;
    void*           UserData;
} print_out;

#define StdOut (print_out){(print_write*)&WriteStdOut, 0}
#define StdErr (print_out){(print_write*)&WriteStdErr, 0}

local usize PrintWrite      (print_out Out, void* Data, usize Size);
local usize PrintCharacter  (print_out Out, char Character);
local usize PrintNewLine    (print_out Out);
local usize Print           (print_out Out, string Message);
local usize Println         (print_out Out, string Message);
local usize PrintUSize      (print_out Out, usize Value);
local usize PrintSSize      (print_out Out, ssize Value);

// ===================================================================================
// NOTE(vak): Implementation
// ===================================================================================

local usize PrintWrite(print_out Out, void* Data, usize Size)
{
    usize Result = Out.Write(Data, Size, Out.UserData);
    return (Result);
}

local usize PrintCharacter(print_out Out, char Character)
{
    usize Written = PrintWrite(Out, &Character, 1);
    return (Written);
}

local usize PrintNewLine(print_out Out)
{
    usize Written = PrintCharacter(Out, '\n');
    return (Written);
}

local usize Print(print_out Out, string Message)
{
    usize Written = PrintWrite(Out, Message.Data, Message.Size);
    return (Written);
}

local usize Println(print_out Out, string Message)
{
    usize Written = 0;

    Written += Print(Out, Message);
    Written += PrintNewLine(Out);

    return (Written);
}

local usize PrintUSize(print_out Out, usize Value)
{
    char Digits[64] = {0};
    usize DigitIndex = ArrayCount(Digits);
    usize DigitCount = 0;

    do
    {
        char Digit = '0' + (char)(Value % 10);
        Value /= 10;

        DigitIndex--;
        DigitCount++;

        Digits[DigitIndex] = Digit;
    } while(Value);

    usize Written = Print(Out, StrData(Digits + DigitIndex, DigitCount));
    return (Written);
}

local usize PrintSSize(print_out Out, ssize Value)
{
    usize Written = 0;

    if (Value < 0)
    {
        Written += PrintCharacter(Out, '-');
        Value = -Value;
    }

    Written += PrintUSize(Out, Value);

    return (Written);
}

