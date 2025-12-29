#ifndef WINDOZER_H
#define WINDOZER_H

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

struct WinDozer {
  // Windows API handles and structures
  KBDLLHOOKSTRUCT kbdStruct;
  HHOOK hKbdHook;
  HWINEVENTHOOK hWinEventHook;
  HANDLE hLockFile{INVALID_HANDLE_VALUE};

  // Data structures
  std::vector<char> inBuff;
  std::map<std::string, std::vector<int>> rectMap;
  std::map<std::string, HWND> winMap;
  std::vector<int> storedGeometry;

  struct WindowSnapshot {
    HWND hWnd;
    std::string className;
    std::string windowTitle;
    std::vector<int> rect;
    bool isMaximized{false};
    int zOrder{0};  // Relative Z-order position (0 = topmost, higher = lower)
  };
  std::map<std::string, std::vector<WindowSnapshot>> layoutSnapshots;
  std::map<std::string, HWND> snapshotFocusedWindow;  // Maps snapshot ID to focused window HWND

  std::string appDataPath;
  std::string settingsPath;

  // Command line arguments / flags
  int bufferSize{20};
  bool disableBufferFlush;
  bool verbose;
  bool debug;
  bool cleanup;
  // Track buffer content that was cleaned (prevents repeating cleanup with dbf)
  std::string lastCleanedBuffer;
  // Used to ignore synthesized backspaces during cleanup
  bool isPerformingCleanup{false};

  // Window adjustment state
  bool adjusting;
  HWND hAdjustedWindow;
  int adjustStep{1};

  // Keyboard / submit settings
  DWORD SUBMIT{VK_RCONTROL};    // For triggeringbuffer evaluation (default: VK_RCONTROL)
  DWORD MODIFIER{VK_LCONTROL};  // For resize mode during adjustment (default: VK_LCONTROL)
  int KBD_REPEAT_RATE{0};       // From registry, for safe cleanUp() in modern/tiled apps.

  // Keyboard input constants
  // GetAsyncKeyState return value: high bit (0x8000) indicates key is currently pressed
  static constexpr SHORT KEY_STATE_PRESSED = 0x8000;
  // Virtual key codes for letters (Windows uses ASCII values)
  static constexpr DWORD VK_A = 'A';
  static constexpr DWORD VK_Z = 'Z';
  // Virtual key codes for numrow (Windows uses ASCII values)
  static constexpr DWORD VK_0 = '0';
  static constexpr DWORD VK_9 = '9';
  // Numpad to numrow offset: VK_NUMPAD0 (96) -> '0' (48)
  static constexpr int NUMPAD_TO_NUMROW_OFFSET = 48;
  // Function key to numrow offset: VK_F1 (112) -> '1' (49), so 112 - 63 = 49
  static constexpr int FUNCTION_KEY_TO_NUMROW_OFFSET = 63;

  // UI / Help
  void printFigletWelcome();
  void printHelp();

  // Rect management
  void loadRectIDs();
  void setRectID(std::string rectID);
  void eraseRectID(std::string rectID);
  void printRects();
  bool saveRectIDs(std::string path);

  // Window ID management
  void setWinID(std::string winID);
  void eraseWinID(std::string winID);
  void printWinIDs();

  // Layout snapshots
  void printSnapshots();
  void saveLayoutSnapshot(std::string snapshotID);
  void restoreLayoutSnapshot(std::string snapshotID);

  // Geometry copy/paste
  void copyGeometry();
  void pasteGeometry();

  // Window pinning (always-on-top)
  void pinThis();

  // Window centering
  void centerThis();

  // Window adjustment
  void enterAdjustWindowMode(std::string winID, int step = 1);
  void exitAdjustWindowMode();
  void adjustWindow(DWORD vkCode, bool isResizeMode);

  // Window movement
  void focusWindow(std::string winID);
  void moveWindow(std::string rectID);                     // Move focused window to rect
  void moveWindow(std::string winID, std::string rectID);  // Move window by ID to rect
  void moveWindowToRect(
    HWND hWnd, const std::vector<int>& rect);  // Internal: helper for multi-monitor DPI handling

  // Buffer management
  void shiftBuffer(char inChar, bool isBackspace = false);
  void initBuffer();
  void flushBuffer();
  void printBuffer();

  // Command parsing
  void interpretBuffer();
  enum class TokenType { COMMAND, NUMBER, IDENTIFIER, END, UNKNOWN };
  struct Token {
    TokenType type;
    std::string value;
    size_t position;
  };
  struct Command {
    std::string name;
    std::vector<std::string> args;
    int commandLength{0};
  };
  std::vector<Token> tokenize(const std::string& input);
  Command parseCommand(const std::vector<Token>& tokens);
  bool executeCommand(const Command& cmd);

  // Input handling
  void ingressInput();
  void cleanUp(std::string match);

  // Internal / utility functions
  bool excludeOthers();
  void cleanupLockFile();  // Clean up lock file on initialization failure
  bool initAppData();
  bool initArgs(int argc, char* argv[]);
  bool readKeyboardRepeatRate();
  bool validWindow(HWND hWnd);
  std::string registered(HWND hWnd);
  std::string getTimestamp();
  void exitWinDozer(int exitCode = EXIT_SUCCESS);
};

#endif  // WINDOZER_H
