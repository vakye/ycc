
// ===================================================================================
// NOTE(vak): Platform-related definitions and functions
// ===================================================================================

#pragma once

// ===================================================================================
// NOTE(vak): Dependencies
// ===================================================================================

#include "shared.c"

// ===================================================================================
// NOTE(vak): Interface
// ===================================================================================

local void*     ReserveMemory       (usize Size);
local b32       CommitMemory        (void* Memory, usize Size);

local void*     MapExecutableMemory (void* Code, usize CodeSize);

local usize     WriteStdOut         (void* Data, usize Size, ...);
local usize     WriteStdErr         (void* Data, usize Size, ...);

local void      Exit                (u8 ExitCode);

// ===================================================================================
// NOTE(vak): Implementations are provided by the platform layer:
//      Linux   -> linux.c
// ===================================================================================

