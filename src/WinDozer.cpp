#include "headers/WinDozer.h"
#include <Windows.h>
#include <psapi.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

std::string WinDozer::getTimestamp() {
  auto now = std::time(nullptr);
  std::tm timeInfo;
  localtime_s(&timeInfo, &now);

  std::ostringstream oss;
  oss << std::setfill('0') << "[" << std::setw(2) << timeInfo.tm_hour << ":" << std::setw(2)
      << timeInfo.tm_min << ":" << std::setw(2) << timeInfo.tm_sec << "] ";
  return oss.str();
}

void WinDozer::printFigletWelcome() {
  std::cout << ("          _          ___                  \n")
            << ("__      _(_)_ __    /   \\___ _______ _ __ \n")
            << ("\\ \\ /\\ / / | '_ \\  / /\\ / _ \\_  / _ \\ '__|\n")
            << (" \\ V  V /| | | | |/ /_// (_) / /  __/ |   \n")
            << ("  \\_/\\_/ |_|_| |_/____/ \\___/___\\___|_|\n\n");
}

// clang-format off
void WinDozer::printHelp() {
  printFigletWelcome();
  std::cout
    << ("Press Ctrl+C or use the 'EXIT' command to exit.\n\n")

    << ("https://github.com/emboiko/winDozer\n\n")

    << ("Flags:\n\n")

    << ("\tdbf\t\t\t\t Disable Buffer Flush\n")
    << ("\tverbose\t\t\t\t Extra console feedback from commands\n")
    << ("\tdebug\t\t\t\t Flood stdout with logs from the buffer\n")
    << ("\tcleanup\t\t\t\t Synthesize backspace keystrokes following valid input evaluation\n")
    << ("\tbs{buffer size}\t\t\t Set buffer size\n")
    << ("\tvks{virtual key #}\t\t Virtual key submit/evaluate\n")
    << ("\tvkm{virtual key #}\t\t Virtual key modifier for adjust/resize mode\n") 

    << ("\nSyntax:\n\n") 
    
    << ("\tSR{Rect ID}\t\t\t Set [focused window's] Rect ID\n")
    << ("\tSW{Window ID}\t\t\t Set Window ID\n")
    << ("\tER{Rect ID}\t\t\t Erase Rect ID\n")
    << ("\tEW{Window ID}\t\t\t Erase Window ID\n")
    << ("\tMTR{Rect ID}\t\t\t Move This [window] to Rect\n")
    << ("\tMW{Window ID}R{Rect ID}\t\t Move Window to Rect\n")
    << ("\tFW{Window ID}\t\t\t Focus Window by ID\n")
    << ("\tAW{Window ID}[S{step}]\t\t Adjust Window by ID (optional step size)\n")
    << ("\tAT[S{step}]\t\t\t Adjust This [window] (optional step size)\n")
    << ("\tSS{Snapshot ID}\t\t\t Save Layout Snapshot\n")
    << ("\tRS{Snapshot ID}\t\t\t Restore Layout Snapshot\n")
    << ("\tPT\t\t\t\t Pin/Unpin This [window] (toggle always-on-top)\n")
    << ("\tCT\t\t\t\t Center This [window] on its current display\n")
    << ("\tGR\t\t\t\t Get/Print all Rects\n")
    << ("\tGW\t\t\t\t Get/Print all Windows\n")
    << ("\tGS\t\t\t\t Get/Print all Layout Snapshots\n")
    << ("\tCG\t\t\t\t Copy/Store focused window's geometry\n")
    << ("\tVG\t\t\t\t Paste/Apply stored geometry to focused window\n")
    << ("\tFLUSH\t\t\t\t Flush Buffer\n")
    << ("\tHELP\t\t\t\t Print this dialog\n")
    << ("\tEXIT\t\t\t\t Exit winDozer\n")
    << ("\t<Submit>\t\t\t Evaluate Buffer (default: <RCtrl>)\n")
    << ("\t<Modifier>\t\t\t Modifier key for adjust/resize mode (default: <LCtrl>)\n");
  }
// clang-format on


void WinDozer::setRectID(std::string rectID) {
  std::vector<int> rect;

  RECT winRect;
  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  GetWindowRect(hActvWnd, &winRect);

  rect.push_back(winRect.left);
  rect.push_back(winRect.top);
  rect.push_back(winRect.bottom);
  rect.push_back(winRect.right);

  rectMap[rectID] = rect;

  if (verbose) {
    std::cout << getTimestamp() << "SET Rect " << rectID << "\n";
  }
}

void WinDozer::eraseRectID(std::string rectID) {
  if (!rectMap.erase(rectID)) {
    std::cout << "No coordinates found for Rect ID: " << rectID << "\n";
  } else {
    if (verbose) {
      std::cout << getTimestamp() << "ERASE Rect " << rectID << "\n";
    }
  };
}

void WinDozer::printRects() {
  if (rectMap.empty()) {
    std::cout << "No registered Rect ID(s) found\n";
    return;
  }

  std::cout << "\n";
  std::cout << "GET Rects:\n";
  std::cout << "------------\n";

  for (auto it = rectMap.begin(); it != rectMap.end(); it++) {
    std::string rectID = it->first;
    std::vector<int> coords = rectMap[rectID];

    if (coords.size() == 4) {
      int x = coords[0];                   // left
      int y = coords[1];                   // top
      int width = coords[3] - coords[0];   // right - left
      int height = coords[2] - coords[1];  // bottom - top

      std::cout << "Rect ID " << rectID << ":\n";
      std::cout << "  X:      " << x << "\n";
      std::cout << "  Y:      " << y << "\n";
      std::cout << "  Width:  " << width << "\n";
      std::cout << "  Height: " << height << "\n";
      std::cout << "\n";
    }
  }
}

void WinDozer::printSnapshots() {
  if (layoutSnapshots.empty()) {
    std::cout << "No layout snapshots found\n";
    return;
  }

  std::cout << "\n";
  std::cout << "GET Snapshots:\n";
  std::cout << "------------\n";

  for (auto it = layoutSnapshots.begin(); it != layoutSnapshots.end(); it++) {
    std::string snapshotID = it->first;
    const std::vector<WindowSnapshot>& snapshot = it->second;

    std::cout << "Snapshot ID " << snapshotID << " (" << snapshot.size() << " windows):\n";
    for (size_t i = 0; i < snapshot.size(); i++) {
      const WindowSnapshot& win = snapshot[i];
      int x = win.rect[0];
      int y = win.rect[1];
      int width = win.rect[3] - win.rect[0];
      int height = win.rect[2] - win.rect[1];

      std::cout << "  [" << (i + 1) << "] " << win.windowTitle << " (" << win.className << ")\n";
      std::cout << "      X: " << x << ", Y: " << y << ", Width: " << width
                << ", Height: " << height << "\n";
    }
    std::cout << "\n";
  }
}

void WinDozer::printWinIDs() {
  if (winMap.empty()) {
    std::cout << "No registered Window ID(s) found\n";
    return;
  }

  char winText[MAX_PATH];
  char classText[MAX_PATH];
  for (auto it = winMap.begin(); it != winMap.end(); it++) {
    std::string winID = it->first;
    GetWindowTextA(winMap[winID], winText, MAX_PATH);
    GetClassNameA(winMap[winID], classText, sizeof(classText));
    std::cout << "Window ID " << winID << ":\t" << winText << "\n"
              << "\t\t" << classText << "\n";
  }
}

void WinDozer::enterAdjustWindowMode(std::string winID, int step) {
  HWND hwnd;
  if (winID.empty()) {
    hwnd = GetForegroundWindow();
    if (!validWindow(hwnd)) {
      adjusting = false;
      return;
    }

    // The currently focused window
    hAdjustedWindow = hwnd;

  } else {
    if (!winMap.count(winID)) {
      std::cout << "No registered windows found for Window ID: " << winID << "\n";

      adjusting = false;
      return;
    }

    // The window with the given ID/handle
    hAdjustedWindow = winMap[winID];
  }

  adjusting = true;
  adjustStep = (step > 0) ? step : 1;  // Ensure step is at least 1
  char winText[MAX_PATH];
  GetWindowTextA(hAdjustedWindow, winText, MAX_PATH);
  if (verbose) {
    std::cout << getTimestamp() << "ADJUST Window " << winText << " (step: " << adjustStep
              << ", hold <Modifier> to resize)\n";
  }
}

void WinDozer::exitAdjustWindowMode() {
  if (verbose) {
    std::cout << getTimestamp() << "Exit Adjustment.\n";
  }
  adjusting = false;
  hAdjustedWindow = NULL;
}

void WinDozer::adjustWindow(DWORD vkCode, bool isResizeMode) {
  // Raise the window without necessarily focusing it
  if (IsIconic(hAdjustedWindow)) {
    ShowWindow(hAdjustedWindow, SW_RESTORE);
  }

  RECT winRect;
  GetWindowRect(hAdjustedWindow, &winRect);
  char winText[MAX_PATH];
  GetWindowTextA(hAdjustedWindow, winText, MAX_PATH);

  int oldX = winRect.left;
  int oldY = winRect.top;
  int oldWidth = winRect.right - winRect.left;
  int oldHeight = winRect.bottom - winRect.top;

  int newX = winRect.left;
  int newY = winRect.top;
  int newWidth = winRect.right - winRect.left;
  int newHeight = winRect.bottom - winRect.top;

  switch (vkCode) {
    case 37:
      // Left
      if (isResizeMode) {
        newWidth -= adjustStep;
      } else {
        newX -= adjustStep;
      }
      break;
    case 38:
      // Up
      if (isResizeMode) {
        newHeight -= adjustStep;
      } else {
        newY -= adjustStep;
      }
      break;
    case 39:
      // Right
      if (isResizeMode) {
        newWidth += adjustStep;
      } else {
        newX += adjustStep;
      }
      break;
    case 40:
      // Down
      if (isResizeMode) {
        newHeight += adjustStep;
      } else {
        newY += adjustStep;
      }
      break;
  }

  // SetWindowPos is used here for consistency with moveWindowToRect
  if (isResizeMode) {
    // Resizes the window with the same location
    SetWindowPos(hAdjustedWindow, NULL, winRect.left, winRect.top, newWidth, newHeight,
                 SWP_NOZORDER | SWP_SHOWWINDOW);
    if (verbose) {
      std::cout << getTimestamp() << "ADJUST Window (resize) " << winText << " from (" << oldWidth
                << ", " << oldHeight << ") to (" << newWidth << ", " << newHeight << ") "
                << "step: " << adjustStep << "\n";
    }
  } else {
    // Moves the window with the same width and height
    SetWindowPos(hAdjustedWindow, NULL, newX, newY, winRect.right - winRect.left,
                 winRect.bottom - winRect.top, SWP_NOZORDER | SWP_SHOWWINDOW);
    if (verbose) {
      std::cout << getTimestamp() << "ADJUST Window (move) " << winText << " from (" << oldX << ", "
                << oldY << ") to (" << newX << ", " << newY << ") " << "step: " << adjustStep
                << "\n";
    }
  }
}

void WinDozer::focusWindow(std::string winID) {
  if (!winMap.count(winID)) {
    std::cout << "No registered windows found for Window ID: " << winID << "\n";
    return;
  }

  // Restore the window if neccesary:
  if (IsIconic(winMap[winID])) {
    ShowWindow(winMap[winID], SW_RESTORE);
  }
  // Perform a magic ritual:
  keybd_event(0, 0, 0, 0);
  // Focus the window:
  SetForegroundWindow(winMap[winID]);
  if (verbose) {
    std::cout << getTimestamp() << "FOCUS Window " << winID << "\n";
  }
}

// Move window to the specified rect coordinates.
//
// NOTE: Multi-monitor DPI scaling issues
// This function uses MoveWindow for simplicity and consistency. However, there are known issues
// when moving windows between monitors with different DPI scaling settings (e.g., primary at 125%
// and secondary at 100%). Windows (win32) may incorrectly interpret size parameters, causing
// windows to grow or shrink unexpectedly.
//
// Attempted solutions that didn't work:
// - Two-step approach (move then resize) with SetWindowPos
// - Detecting DPI scaling factors and adjusting target sizes
// - Feedback-based correction after resize (this gets messy and inconsistent)
//
// The root issue: GetWindowRect returns DPI-unaware virtual screen coordinates, but SetWindowPos
// and MoveWindow interpret size parameters based on the DPI of the monitor where the window is.
// When moving between monitors with different DPI, the size interpretation can be inconsistent.
//
// For now, we accept that mixed DPI scaling in multi-monitor setups may cause sizing issues. See
// readme.md for more details.
void WinDozer::moveWindowToRect(HWND hWnd, const std::vector<int>& rect) {
  int targetWidth = rect[3] - rect[0];   // right - left
  int targetHeight = rect[2] - rect[1];  // bottom - top

  MoveWindow(hWnd, rect[0], rect[1], targetWidth, targetHeight, TRUE);
}

// Move This [window] to Rect {Rect ID}
void WinDozer::moveWindow(std::string rectID) {
  if (!rectMap.count(rectID)) {
    std::cout << "No registered rects found for Rect ID: " << rectID << "\n";
    return;
  }

  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  ShowWindow(hActvWnd, SW_RESTORE);  // Window won't move if it's maximized
  moveWindowToRect(hActvWnd, rectMap[rectID]);

  if (verbose) {
    std::cout << getTimestamp() << "MOVE [focused] Window to Rect " << rectID << "\n";
  }
}

// Move a [window] by its {Window ID} to the rect described by a {Rect ID}
void WinDozer::moveWindow(std::string winID, std::string rectID) {
  if (!winMap.count(winID)) {
    std::cout << "No registered windows found for Window ID: " << winID << "\n";
    return;
  }
  if (!rectMap.count(rectID)) {
    std::cout << "No registered rects found for Rect ID: " << rectID << "\n";
    return;
  }

  ShowWindow(winMap[winID], SW_RESTORE);  // Restore the window if it's minimized
  moveWindowToRect(winMap[winID], rectMap[rectID]);

  if (verbose) {
    std::cout << getTimestamp() << "MOVE Window " << winID << " to Rect " << rectID << "\n";
  }
}

void WinDozer::setWinID(std::string winID) {
  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  winMap[winID] = hActvWnd;
  if (verbose) {
    std::cout << getTimestamp() << "SET Window ID " << winID << "\n";
  }
}

void WinDozer::eraseWinID(std::string winID) {
  if (!winMap.erase(winID)) {
    std::cout << "No registered windows found for Window ID: " << winID << "\n";
  } else {
    if (verbose) {
      std::cout << getTimestamp() << "ERASE Window ID: " << winID << "\n";
    }
  };
}

// Helper function to detect if a window belongs to a modern/UWP app
// Modern apps are hosted by ApplicationFrameHost.exe, which we can detect
// by checking the process executable name
static bool isModernApp(HWND hWnd) {
  DWORD processId = 0;
  GetWindowThreadProcessId(hWnd, &processId);
  if (processId == 0) {
    return true;  // Can't determine, assume modern
  }

  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
  if (!hProcess) {
    return true;  // Can't open process, assume modern
  }

  char exePath[MAX_PATH];
  bool isModern = true;
  if (GetModuleFileNameExA(hProcess, NULL, exePath, MAX_PATH)) {
    // Extract just the filename from the path
    const char* exeName = strrchr(exePath, '\\');
    exeName = exeName ? exeName + 1 : exePath;
    // UWP apps are hosted by ApplicationFrameHost.exe
    if (_stricmp(exeName, "ApplicationFrameHost.exe") == 0) {
      isModern = true;
    } else {
      isModern = false;
    }
  }

  CloseHandle(hProcess);
  return isModern;
}

void WinDozer::exitWinDozer(int exitCode) {
  // Save settings
  saveRectIDs(settingsPath);

  // Unhook keyboard and window event hooks
  if (hKbdHook != NULL) {
    UnhookWindowsHookEx(hKbdHook);
  }
  if (hWinEventHook != NULL) {
    UnhookWinEvent(hWinEventHook);
  }

  // Clean up lock file
  cleanupLockFile();

  // Exit with the provided exit code
  exit(exitCode);
}

void WinDozer::cleanUp(std::string command) {
  if (cleanup) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    input.ki.wVk = VK_BACK;

    // Get the currently focused window to determine if delay is needed
    HWND hForeground = GetForegroundWindow();
    bool delayNeeded = isModernApp(hForeground);

    // Backspace-based cleanup with adaptive delay.
    // Modern apps (Sticky Notes, Microsoft Store, etc.) treat rapid backspaces
    // like a held key, causing accelerated deletion. We add a delay between
    // backspaces to stay below the repeat rate threshold.
    // Classic apps (Notepad, etc.) work fine without delay, so we skip it for speed.
    // Note: Cleanup is an ATTEMPT - if focus changed, results may vary.
    for (size_t i = 0; i < command.size(); i++) {
      input.ki.dwFlags = 0;
      SendInput(1, &input, sizeof(INPUT));
      input.ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &input, sizeof(INPUT));
      if (delayNeeded) {
        Sleep(KBD_REPEAT_RATE);
      }
    }
  }
}

bool WinDozer::validWindow(HWND hWnd) {
  char classText[MAX_PATH];
  GetClassNameA(hWnd, classText, sizeof(classText));
  std::string className = classText;

  if (debug) {
    std::cout << "Validating Window class: " << className << "\n";
  }

  // Non-dozable windows:
  if (className == "Windows.UI.Core.CoreWindow" ||  // The start menu
      className == "Shell_TrayWnd" ||               // The system tray
      className == "Progman" ||                     // The desktop itself / child context menu
      className == "Program Manager" ||             // The desktop itself / child context menu
      className == "WorkerW"                        // The desktop itself / child context menu
  ) {
    if (debug) {
      std::cout << "Invalid Window Class: " << className << "\n";
    }
    return false;
  }

  return true;
}

std::string WinDozer::registered(HWND hWnd) {
  if (winMap.empty()) {
    return "";
  }

  std::string winID;
  for (auto it = winMap.begin(); it != winMap.end(); it++) {
    winID = it->first;
    if (winMap[winID] == hWnd)
      return winID;
  }

  return "";
}

void WinDozer::copyGeometry() {
  RECT winRect;
  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  GetWindowRect(hActvWnd, &winRect);

  storedGeometry.clear();
  storedGeometry.push_back(winRect.left);
  storedGeometry.push_back(winRect.top);
  storedGeometry.push_back(winRect.bottom);
  storedGeometry.push_back(winRect.right);

  if (verbose) {
    std::cout << getTimestamp() << "COPY Geometry from focused window\n";
  }
}

void WinDozer::pasteGeometry() {
  if (storedGeometry.empty() || storedGeometry.size() != 4) {
    std::cout << "No geometry stored. Use CG to copy a window's geometry first.\n";
    return;
  }

  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  ShowWindow(hActvWnd, SW_RESTORE);  // Window won't resize if it's maximized

  // Get current window position (we'll keep this)
  RECT currentRect;
  GetWindowRect(hActvWnd, &currentRect);
  int currentX = currentRect.left;
  int currentY = currentRect.top;

  // Calculate width and height from stored geometry
  int targetWidth = storedGeometry[3] - storedGeometry[0];   // right - left
  int targetHeight = storedGeometry[2] - storedGeometry[1];  // bottom - top

  // Resize only, keeping current position
  SetWindowPos(hActvWnd, NULL, currentX, currentY, targetWidth, targetHeight,
               SWP_NOZORDER | SWP_SHOWWINDOW);

  if (verbose) {
    std::cout << getTimestamp() << "PASTE Geometry to focused window\n";
  }
}

void WinDozer::pinThis() {
  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  // Check if window is currently topmost
  LONG exStyle = GetWindowLongPtr(hActvWnd, GWL_EXSTYLE);
  bool isTopmost = (exStyle & WS_EX_TOPMOST) != 0;

  if (isTopmost) {
    // Remove topmost status
    SetWindowPos(hActvWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (verbose) {
      char winText[MAX_PATH];
      GetWindowTextA(hActvWnd, winText, MAX_PATH);
      std::cout << getTimestamp() << "UNPIN Window " << winText << "\n";
    }
  } else {
    // Set as topmost
    SetWindowPos(hActvWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (verbose) {
      char winText[MAX_PATH];
      GetWindowTextA(hActvWnd, winText, MAX_PATH);
      std::cout << getTimestamp() << "PIN Window " << winText << "\n";
    }
  }
}

void WinDozer::centerThis() {
  HWND hActvWnd = GetForegroundWindow();
  if (!validWindow(hActvWnd)) {
    return;
  }

  // Restore if minimized (can't center a minimized window)
  if (IsIconic(hActvWnd)) {
    ShowWindow(hActvWnd, SW_RESTORE);
  }

  // Get current window rect
  RECT winRect;
  GetWindowRect(hActvWnd, &winRect);

  // Find the center point of the window
  POINT center;
  center.x = (winRect.left + winRect.right) / 2;
  center.y = (winRect.top + winRect.bottom) / 2;

  // Find which monitor contains the window's center point
  HMONITOR hMonitor = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
  if (hMonitor == NULL) {
    return;  // Can't find monitor, abort
  }

  // Get monitor info (work area excludes taskbar)
  MONITORINFO monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFO);
  if (!GetMonitorInfo(hMonitor, &monitorInfo)) {
    return;  // Can't get monitor info, abort
  }

  // Calculate window dimensions
  int windowWidth = winRect.right - winRect.left;
  int windowHeight = winRect.bottom - winRect.top;

  // Calculate work area dimensions
  int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
  int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

  // Center the window in the work area
  int newX = monitorInfo.rcWork.left + (workWidth - windowWidth) / 2;
  int newY = monitorInfo.rcWork.top + (workHeight - windowHeight) / 2;

  // Move the window to the centered position
  SetWindowPos(hActvWnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

  if (verbose) {
    char winText[MAX_PATH];
    GetWindowTextA(hActvWnd, winText, MAX_PATH);
    std::cout << getTimestamp() << "CENTER Window " << winText << "\n";
  }
}

void WinDozer::saveLayoutSnapshot(std::string snapshotID) {
  // Clear any existing snapshot with this ID
  layoutSnapshots[snapshotID].clear();

  // Store the currently focused window for this snapshot
  HWND hFocused = GetForegroundWindow();
  if (validWindow(hFocused)) {
    snapshotFocusedWindow[snapshotID] = hFocused;
  } else {
    snapshotFocusedWindow[snapshotID] = NULL;
  }

  // Structure to pass data to EnumWindows callback
  struct EnumData {
    WinDozer* winDozer;
    std::vector<WindowSnapshot>* windows;
    int zOrderCounter;
  };

  std::vector<WindowSnapshot> collectedWindows;
  EnumData enumData = {this, &collectedWindows, 0};

  // Enumerate all windows and collect valid, non-minimized ones
  // EnumWindows enumerates from top to bottom in Z-order
  // Return true to continue enumeration, false to stop
  EnumWindows(
    [](HWND hWnd, LPARAM lParam) -> BOOL {
      EnumData* data = reinterpret_cast<EnumData*>(lParam);

      // Skip invalid windows (system windows, desktop, etc.)
      if (!data->winDozer->validWindow(hWnd)) {
        return TRUE;
      }

      // Skip windows that aren't visible
      if (!IsWindowVisible(hWnd)) {
        return TRUE;
      }

      // Skip minimized windows
      if (IsIconic(hWnd)) {
        return TRUE;
      }

      // Skip tool windows (floating toolbars, etc.) - these aren't main application windows
      // Tool windows have the WS_EX_TOOLWINDOW extended style
      LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
      if (exStyle & WS_EX_TOOLWINDOW) {
        return TRUE;
      }

      // Skip windows that have a parent (they're child windows, not top-level)
      if (GetParent(hWnd) != NULL) {
        return TRUE;
      }

      // Get window geometry first to check size
      RECT winRect;
      GetWindowRect(hWnd, &winRect);
      int width = winRect.right - winRect.left;
      int height = winRect.bottom - winRect.top;

      // Skip windows that are too small (likely not real application windows)
      // Most real application windows are at least 100x100 pixels
      if (width < 100 || height < 100) {
        // Worst case, we could whitelist tiny windows here if we need to
        return TRUE;
      }

      // Get window class and title
      char className[MAX_PATH];
      char windowTitle[MAX_PATH];
      GetClassNameA(hWnd, className, sizeof(className));
      GetWindowTextA(hWnd, windowTitle, sizeof(windowTitle));

      // Skip windows with no title (many system/hidden windows have empty titles)
      // But allow windows with titles even if they're short
      if (strlen(windowTitle) == 0) {
        return TRUE;
      }

      // Create snapshot entry
      WindowSnapshot snapshot;
      snapshot.hWnd = hWnd;  // Store HWND as primary identifier
      snapshot.className = className;
      snapshot.windowTitle = windowTitle;
      snapshot.rect.push_back(winRect.left);
      snapshot.rect.push_back(winRect.top);
      snapshot.rect.push_back(winRect.bottom);
      snapshot.rect.push_back(winRect.right);
      snapshot.isMaximized = IsZoomed(hWnd) != 0;  // Check if window is maximized
      snapshot.zOrder = data->zOrderCounter++;     // Store Z-order (0 = topmost)

      data->windows->push_back(snapshot);
      return TRUE;
    },
    reinterpret_cast<LPARAM>(&enumData));

  // Store the collected windows in the snapshot map
  layoutSnapshots[snapshotID] = collectedWindows;

  if (verbose) {
    std::cout << getTimestamp() << "SAVE Layout Snapshot " << snapshotID << " ("
              << collectedWindows.size() << " windows)\n";
  }
}

void WinDozer::restoreLayoutSnapshot(std::string snapshotID) {
  // Check if snapshot exists
  if (layoutSnapshots.find(snapshotID) == layoutSnapshots.end()) {
    std::cout << "No layout snapshot found for ID: " << snapshotID << "\n";
    return;
  }

  const std::vector<WindowSnapshot>& snapshot = layoutSnapshots[snapshotID];
  if (snapshot.empty()) {
    std::cout << "Layout snapshot " << snapshotID << " is empty\n";
    return;
  }

  int restored = 0;
  int notFound = 0;
  std::vector<std::pair<HWND, const WindowSnapshot*>> matchedWindows;

  // Step 1: Match windows and collect matches
  for (const auto& windowSnapshot : snapshot) {
    HWND matchedWindow = NULL;

    // First, try to match by HWND (primary identifier)
    // Check if the stored HWND is still valid and refers to the same window
    if (IsWindow(windowSnapshot.hWnd)) {
      // HWND is still valid - verify it's the same window by comparing class
      // (HWNDs are unique, but we verify to be safe)
      char currentClassName[MAX_PATH];
      GetClassNameA(windowSnapshot.hWnd, currentClassName, sizeof(currentClassName));

      // If class matches (title might have changed, e.g., browser tabs), use the HWND
      if (windowSnapshot.className == currentClassName) {
        matchedWindow = windowSnapshot.hWnd;
      }
    }

    // If HWND matching failed (window was closed/reopened), fall back to class+title matching
    if (matchedWindow == NULL) {
      // Structure to pass data to EnumWindows callback for matching
      struct MatchData {
        WinDozer* winDozer;
        const WindowSnapshot* target;
        HWND* found;
      };

      MatchData matchData = {this, &windowSnapshot, &matchedWindow};

      // Enumerate all windows and try to find a match by class + title
      EnumWindows(
        [](HWND hWnd, LPARAM lParam) -> BOOL {
          MatchData* data = reinterpret_cast<MatchData*>(lParam);

          // Skip invalid windows
          if (!data->winDozer->validWindow(hWnd)) {
            return TRUE;
          }

          // Get window class and title
          char className[MAX_PATH];
          char windowTitle[MAX_PATH];
          GetClassNameA(hWnd, className, sizeof(className));
          GetWindowTextA(hWnd, windowTitle, sizeof(windowTitle));

          // Check if this window matches our target by class + title
          if (data->target->className == className && data->target->windowTitle == windowTitle) {
            *data->found = hWnd;
            return FALSE;  // Stop enumeration (found match)
          }

          return TRUE;
        },
        reinterpret_cast<LPARAM>(&matchData));
    }

    if (matchedWindow != NULL) {
      matchedWindows.push_back({matchedWindow, &windowSnapshot});
      restored++;
    } else {
      notFound++;
    }
  }

  // Step 2: Restore window positions (but not maximized state yet)
  for (const auto& windowPair : matchedWindows) {
    HWND hWnd = windowPair.first;
    const WindowSnapshot* windowSnapshot = windowPair.second;

    // Restore if minimized, but don't maximize yet
    if (IsIconic(hWnd)) {
      ShowWindow(hWnd, SW_RESTORE);
    }

    // Restore position and size (if not maximized, we'll maximize later)
    if (!windowSnapshot->isMaximized) {
      moveWindowToRect(hWnd, windowSnapshot->rect);
    }
  }

  // Step 3: Restore Z-order (restore in reverse order: bottom to top)
  // This ensures the topmost window (zOrder=0) is restored last
  std::vector<std::pair<HWND, const WindowSnapshot*>> sortedWindows = matchedWindows;
  std::sort(sortedWindows.begin(), sortedWindows.end(),
            [](const std::pair<HWND, const WindowSnapshot*>& a,
               const std::pair<HWND, const WindowSnapshot*>& b) {
              return a.second->zOrder > b.second->zOrder;  // Higher zOrder first (bottom to top)
            });

  for (const auto& windowPair : sortedWindows) {
    HWND hWnd = windowPair.first;
    // Bring window to top (restoring Z-order from bottom to top)
    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }

  // Step 4: Restore maximized state (after position is set)
  for (const auto& windowPair : matchedWindows) {
    HWND hWnd = windowPair.first;
    const WindowSnapshot* windowSnapshot = windowPair.second;

    if (windowSnapshot->isMaximized) {
      // First restore to normal position, then maximize
      moveWindowToRect(hWnd, windowSnapshot->rect);
      ShowWindow(hWnd, SW_MAXIMIZE);
    }
  }

  // Step 5: Attempt to restore focus to the saved focused window
  HWND hFocused = NULL;
  if (snapshotFocusedWindow.find(snapshotID) != snapshotFocusedWindow.end()) {
    hFocused = snapshotFocusedWindow[snapshotID];
  }

  if (hFocused != NULL && IsWindow(hFocused)) {
    // Verify the window is still valid and was restored
    bool wasRestored = false;
    for (const auto& windowPair : matchedWindows) {
      if (windowPair.first == hFocused) {
        wasRestored = true;
        break;
      }
    }

    if (wasRestored) {
      // Magic ritual to allow SetForegroundWindow
      keybd_event(0, 0, 0, 0);
      SetForegroundWindow(hFocused);
    }
  }

  // If focus restoration failed, focus the last window (topmost in Z-order)
  if (hFocused == NULL || !IsWindow(hFocused) || GetForegroundWindow() != hFocused) {
    if (!matchedWindows.empty()) {
      // Focus the window with the lowest zOrder (topmost)
      HWND topWindow = NULL;
      int minZOrder = INT_MAX;
      for (const auto& windowPair : matchedWindows) {
        if (windowPair.second->zOrder < minZOrder) {
          minZOrder = windowPair.second->zOrder;
          topWindow = windowPair.first;
        }
      }
      if (topWindow != NULL) {
        keybd_event(0, 0, 0, 0);
        SetForegroundWindow(topWindow);
      }
    }
  }

  if (verbose) {
    std::cout << getTimestamp() << "RESTORE Layout Snapshot " << snapshotID
              << " (restored: " << restored << ", not found: " << notFound << ")\n";
  }
}
