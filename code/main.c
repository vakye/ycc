
// ===================================================================================
// NOTE(vak): Main program logic. Contains the Main() function.
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"
#include "print.c"
#include "error.c"
#include "lexer.c"
#include "parser.c"
#include "generator.c"

// ===================================================================================
// NOTE(vak): Main function
// ===================================================================================

typedef ssize program_main(void);

local void Main(void)
{

#if !Architecture_X64
    #error Sorry, only x64 (x86_64) is implemented at the moment
#endif

    string Code = Str("((  (1000 + 400) - (200+200)) +(200+30))   +  (7-  (3  + (4-2) - 2))");

    SetupLexer();
    SetupParser();
    SetupGenerator();

    ResetLexer();
    ResetParser();
    ResetGenerator();

    token_array Tokens = Tokenize(Code);

    {
        Println(StdOut, Str("Tokenizer output:"));
        PrintTokens(Tokens);
    }

    node_id RootNode = Parse(Tokens);

    {
        Println(StdOut, Str("Parser output:"));
        PrintNode(RootNode);
    }

    gen_block GenBlock = x64_Generate(RootNode);

    void* ExecutableMemory = MapExecutableMemory(GenBlock.Code, GenBlock.CodeSize);

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

