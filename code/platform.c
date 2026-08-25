
#pragma once

local void* MapExecutableMemory(void* Code, usize CodeSize);

local usize WriteStdOut(void* Data, usize Size, ...);
local usize WriteStdErr(void* Data, usize Size, ...);

local void Exit(u8 ExitCode);

