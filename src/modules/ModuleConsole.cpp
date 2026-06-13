#include "ModuleConsole.h"
#include "VM.h"
#include "ModuleUtil.h"
#include <iostream>
#include <string>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

namespace Flux::Modules {

void handleConsole(const std::string& subName, VM& vm) {
    if (subName == "clear") {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    } else if (subName == "title") {
#ifdef _WIN32
        SetConsoleTitleW(utf8ToWide(Runtime::valueToString(vm.peek(0))).c_str());
#endif
    } else if (subName == "color") {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)std::get<int>(vm.peek(0)));
#endif
    } else if (subName == "input") {
        std::string prompt = Runtime::valueToString(vm.peek(0));
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        vm.pop(); vm.pop(); vm.push(input); return;
    } else if (subName == "width") {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi; GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        vm.pop(); vm.push(csbi.srWindow.Right - csbi.srWindow.Left + 1);
#else
        vm.pop(); vm.push(80);
#endif
        return;
    } else if (subName == "height") {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi; GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        vm.pop(); vm.push(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
#else
        vm.pop(); vm.push(24);
#endif
        return;
    } else if (subName == "cursor") {
#ifdef _WIN32
        int x = std::get<int>(vm.peek(0)); int y = std::get<int>(vm.peek(1));
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{(SHORT)x, (SHORT)y});
#endif
        vm.pop(); vm.pop(); vm.pop(); vm.push(0); return;
    } else if (subName == "readkey") {
#ifdef _WIN32
        char ch = (char)_getch();
        if (ch == -32 || ch == 0) { _getch(); } // discard extended key code
        vm.pop(); vm.push(std::string(1, ch));
#else
        vm.pop(); vm.push(std::string(""));
#endif
        return;
    } else if (subName == "beep") {
#ifdef _WIN32
        Beep(800, 200);
#else
        printf("\a");
#endif
        vm.pop(); vm.push(0); return;
    } else if (subName == "reset") {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#endif
        vm.pop(); vm.push(0); return;
    }
    vm.pop(); vm.pop(); vm.push(0);
    // No break — caller handles it
}

} // namespace Flux::Modules
