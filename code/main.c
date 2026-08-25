
#pragma once

local void Main(void)
{
    char MessageForStdOut[] = "Hello, world from stdout!\n";
    char MessageForStdErr[] = "Hello, world from stderr!\n";

    WriteStdOut(MessageForStdOut, sizeof(MessageForStdOut) - 1);
    WriteStdErr(MessageForStdErr, sizeof(MessageForStdErr) - 1);
}

