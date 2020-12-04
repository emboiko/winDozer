#include <signal.h>
#include <Windows.h>
#include "headers/WinDozer.h"


WinDozer winDozer;


LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        if (wParam == WM_KEYDOWN) {
            winDozer.kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
            winDozer.ingressInput();
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}


void exitHandler(int SIG) {
    winDozer.printRectIDs(WinDozer::FILE, winDozer.settings);
    UnhookWindowsHookEx(winDozer.hHook);
    exit(SIG);
}


int main(int argc, char* argv[]) {
    if (
        (winDozer.hHook = SetWindowsHookExA(
            WH_KEYBOARD_LL,
            hookProc,
            // In the win32api docs:
            // The hMod parameter must be set to NULL if the dwThreadId parameter specifies
            // a thread created by the current process and if the hook procedure is within 
            // the code associated with the current process.
            NULL,
            // Magic number 0 is a thread ID shortcut:
            // if this parameter is zero, the hook procedure is associated with all 
            // existing threads running in the same desktop as the calling thread
            0))
        ) {
        winDozer.initAppData();
        winDozer.excludeOthers();
        winDozer.initArgs(argc, argv);
        winDozer.loadRectIDs();
        winDozer.printFigletWelcome();
        winDozer.flushBuffer();
        signal(SIGINT, exitHandler);
        signal(SIGBREAK, exitHandler);
        LPMSG msg{};
        while (GetMessageW(msg, NULL, 0, 0)) {} // Hold the terminal open
    }
    else {
        std::cerr << (stderr, "Hook failed to set. ");
        std::cerr << (stderr, "Press <Enter> to exit.\n");
        getchar();
    }
    return 0;
}
