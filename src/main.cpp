#include <Windows.h>
#include <signal.h>
#include "headers/WinDozer.h"

WinDozer winDozer;

// Callback for when keyboard events are triggered - populates the input buffer
// code is the hook code, wParam is the window message, lParam is the keyboard state
LRESULT CALLBACK kbdHookProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code >= HC_ACTION) {
    if (wParam == WM_KEYDOWN) {
      // lParam is a pointer-sized integer containing the address of a KBDLLHOOKSTRUCT
      // 1: Cast lParam to KBDLLHOOKSTRUCT* (treat the address as a struct pointer)
      // 2: Dereference with * to get the actual struct value
      // 3: Copy the struct into our member variable
      winDozer.kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
      winDozer.ingressInput();
    }
  }
  // Call the next hook in the hook chain
  // Important if multiple hooks are installed
  // Args: (Handle to the current process, hook code, window message, keyboard state)
  return CallNextHookEx(NULL, code, wParam, lParam);
}

// Callback for when windows are destroyed
// This is used to remove windows from winMap when they are destroyed
void CALLBACK winEventProc(
  [[maybe_unused]] HWINEVENTHOOK hWinEventHook,  // The hook that was used to receive the event
  DWORD event,                                   // The event that occurred
  HWND hwnd,                                     // The window that was destroyed
  LONG idObject,                                 // The object that was destroyed
  [[maybe_unused]] LONG idChild,                 // The child object that was destroyed
  [[maybe_unused]] DWORD idEventThread,          // The thread that the event occurred in
  [[maybe_unused]] DWORD dwmsEventTime) {        // The time the event occurred
  std::string winID;

  if ((event == EVENT_OBJECT_DESTROY) && (idObject == OBJID_WINDOW)) {
    winID = winDozer.registered(hwnd);
    if (!winID.empty())
      winDozer.eraseWinID(winID);
  }
}

// Save settings, unhook, unlock, and exit
// Delegates to winDozer.exitWinDozer() to avoid code duplication
void exitHandler(int SIG) {
  winDozer.exitWinDozer(SIG);
}

int main(int argc, char* argv[]) {
  if (
    // Set the keyboard hook
    (winDozer.hKbdHook = SetWindowsHookExA(
       // The type of hook being installed:
       WH_KEYBOARD_LL,
       // The callback function to be invoked when the hook is triggered:
       kbdHookProc,
       // A handle to the DLL containing the hook procedure pointed to by the lpfn parameter
       // In the win32api docs:
       // The hMod parameter must be set to NULL if the dwThreadId parameter specifies
       // a thread created by the current process and if the hook procedure is within
       // the code associated with the current process.
       NULL,
       // Thread ID shortcut: Magic number 0 is a thread ID shortcut:
       // For desktop apps, if this parameter is zero, the hook procedure is associated with
       // all existing threads running in the same desktop as the calling thread.
       0))

    &&

    // Set the window event hook
    // Doing this with SetWindowsHookEx() is a circus and requires dll injection
    (winDozer.hWinEventHook = SetWinEventHook(
       EVENT_OBJECT_DESTROY,  // Event minimum
       EVENT_OBJECT_DESTROY,  // Event maximum
       NULL,                  // No DLL containing the callback
       winEventProc,          // The callback function to be invoked when the hook is triggered
       0,                     // receive events from all processes on the current desktop
       0,                     // all existing threads on the current desktop
       WINEVENT_OUTOFCONTEXT  // The callback function is not mapped into the address space of the
                              // process that generates the event
       ))) {

    if (!winDozer.initAppData()) {
      return ERROR_BAD_ENVIRONMENT;
    }

    if (!winDozer.excludeOthers()) {
      return ERROR_SHARING_VIOLATION;
    }

    if (!winDozer.initArgs(argc, argv)) {
      return ERROR_INVALID_COMMAND_LINE;
    }

    winDozer.loadRectIDs();
    winDozer.initBuffer();
    winDozer.printFigletWelcome();

    signal(SIGINT, exitHandler);
    signal(SIGBREAK, exitHandler);

    // Message loop required for Windows hooks to function
    // GetMessage blocks until a message arrives, keeping the thread alive
    // Hook callbacks are dispatched through the message queue
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
      // Empty loop body is fine - we just need the message pump running
      // Low-level hooks are called directly by Windows, not through DispatchMessage
      // PeekMessage also works if we need to do other work in the message loop
    }
  } else {
    std::cerr << "Hook failed to set. (Error " << GetLastError() << ")\n"
              << "Press <Enter> to exit.\n";

    getchar();
  }

  return EXIT_SUCCESS;
}
