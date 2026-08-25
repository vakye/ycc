
#pragma once

local usize PrintCharacter(char Character)
{
    usize Written = WriteStdOut(&Character, 1);
    return (Written);
}

local usize PrintNewLine(void)
{
    usize Written = PrintCharacter('\n');
    return (Written);
}

local usize Print(string Message)
{
    usize Written = WriteStdOut(Message.Data, Message.Size);
    return (Written);
}

local usize Println(string Message)
{
    usize Written = 0;

    Written += Print(Message);
    Written += PrintNewLine();

    return (Written);
}

local usize PrintUSize(usize Value)
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

    usize Written = Print(StrData(Digits + DigitIndex, DigitCount));
    return (Written);
}

local usize PrintSSize(ssize Value)
{
    usize Written = 0;

    if (Value < 0)
    {
        Written += PrintCharacter('-');
        Value = -Value;
    }

    Written += PrintUSize(Value);

    return (Written);
}

