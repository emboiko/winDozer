#include <signal.h>
#include <Windows.h>
#include "headers/WinDozer.h"


WinDozer winDozer;


LRESULT CALLBACK kbdHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        if (wParam == WM_KEYDOWN) {
            winDozer.kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
            winDozer.ingressInput();
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}


void CALLBACK winEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime
) {
    std::string winID;

    if ((event == EVENT_OBJECT_DESTROY) && (idObject == OBJID_WINDOW)) {
        winID = winDozer.registered(hwnd);
        if (!winID.empty()) winDozer.eraseWinID(winID);
    }
}


void exitHandler(int SIG) {
    winDozer.printRectIDs(WinDozer::FILE, winDozer.settings);
    UnhookWindowsHookEx(winDozer.hKbdHook);
    UnhookWinEvent(winDozer.hWinEventHook);
    exit(SIG);
}


int main(int argc, char* argv[]) {
    if (
        (winDozer.hKbdHook = SetWindowsHookExA(
            WH_KEYBOARD_LL,
            kbdHookProc,
            // In the win32api docs:
            // The hMod parameter must be set to NULL if the dwThreadId parameter specifies
            // a thread created by the current process and if the hook procedure is within 
            // the code associated with the current process.
            NULL,
            // Magic number 0 is a thread ID shortcut:
            // if this parameter is zero, the hook procedure is associated with all 
            // existing threads running in the same desktop as the calling thread
            0))
        &&
        // Doing this with SetWindowsHookEx() is a circus and requires dll injection
        (winDozer.hWinEventHook = SetWinEventHook(
            EVENT_OBJECT_DESTROY,
            EVENT_OBJECT_DESTROY,
            NULL, // No DLL containing the callback
            winEventProc,
            0, // receive events from all processes on the current desktop.
            0, // all existing threads on the current desktop
            WINEVENT_OUTOFCONTEXT // No DLL
        ))
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
        std::cerr << "Hook failed to set. (Error "
            << GetLastError() << ")\n"
            << "Press <Enter> to exit.\n";

        getchar();
    }
    return 0;
}
