#!/bin/sh

# Build Flux interpreter
g++ -std=c++20 -Iinclude \
    src/main.cpp \
    src/Lexer.cpp \
    src/Parser.cpp \
    src/Compiler.cpp \
    src/VM.cpp \
    -o flux

# Build Flux package manager
g++ -std=c++20 -Iinclude \
    flux-pkg/main.cpp \
    -o flux-pkg

echo
echo "Done. flux and flux-pkg built."