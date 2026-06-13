@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
cl /std:c++20 /utf-8 /EHsc /Iinclude src/main.cpp src/Lexer.cpp src/Parser.cpp src/Compiler.cpp src/VM.cpp /Feflux_interpreter.exe
