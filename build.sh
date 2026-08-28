#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

SourceFile="code/test_linux.c"
OutputFile="build/test_ycc"

CompileFlags=" \
    -g \
    -O0 \
    -ffreestanding \
    -fno-stack-protector \
    -fpie \
    -nostdlib \
    -std=c11 \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-function \
    -Wno-missing-field-initializers \
    -Wno-switch \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,-entry,entry_point"

clang $CompileFlags $LinkFlags $SourceFile

