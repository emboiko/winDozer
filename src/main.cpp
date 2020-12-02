#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <map>
#include <vector>
#include <signal.h>
#include <Windows.h>

#define STDOUT 0
#define FILE 1

KBDLLHOOKSTRUCT kbdStruct;
HHOOK hHook;

const short BUFFSIZE{ 7 };
char inBuff[BUFFSIZE];

std::map<std::string, std::vector<int>> rectMap;
std::map<std::string, HWND> winMap;

std::string appData;
std::string settings;

bool disableBufferFlush{ false };
bool verbose{ false };
bool debugBuffer{ false };


void printFigletWelcome() {
    std::cout << ("          _          ___                  \n");
    std::cout << ("__      _(_)_ __    /   \\___ _______ _ __ \n");
    std::cout << ("\\ \\ /\\ / / | '_ \\  / /\\ / _ \\_  / _ \\ '__|\n");
    std::cout << (" \\ V  V /| | | | |/ /_// (_) / /  __/ |   \n");
    std::cout << ("  \\_/\\_/ |_|_| |_/____/ \\___/___\\___|_|\n\n");
    std::cout << ("Press Ctrl+C to exit.\n\n");
}


void printHelp() {} // coming soon


void loadRectIDs() {
    std::string rectCoord;
    std::string rectID;
    std::vector<int> rect;
    std::regex reRectID{ "Rect ID\\s{1}(\\d+){1}:{1}" };

    std::string line;
    std::fstream inFile(settings);
    while (std::getline(inFile, line)) {

        if (std::regex_match(line, reRectID)) {
            line.erase(0, 8); // trim prefix
            line.erase(line.length() - 1, 1); // trim suffix
            rectID = line;

            rect.erase(rect.begin(), rect.end());

            // Walk the next four lines for that rect ID and collect coords
            for (int i = 0; i < 4; i++) {
                std::getline(inFile, rectCoord);
                rect.push_back(
                    std::stoi(rectCoord)
                );
            }

            rectMap[rectID] = rect;
        }

    }

    inFile.close();
}


void setRectID(std::string rectID) {
    std::vector<int> rect;

    RECT winRect;
    HWND hActvWnd = GetForegroundWindow();
    GetWindowRect(hActvWnd, &winRect);

    rect.push_back(winRect.left);
    rect.push_back(winRect.top);
    rect.push_back(winRect.bottom);
    rect.push_back(winRect.right);

    rectMap[rectID] = rect;

    std::cout << "SET RectID " << rectID << "\n\n";
}


void printRectIDs(int outMode = STDOUT, std::string path = "") {
    if (rectMap.empty()) {
        std::cout << "No registered Rect ID(s) found\n";
    }

    if (outMode == FILE) freopen(path.c_str(), "w", stdout);

    for (auto it = rectMap.begin(); it != rectMap.end(); it++) {
        std::string rectID = it->first;
        std::cout << "Rect ID " << rectID << ":\n";
        for (int coordinate : rectMap[rectID]) {
            std::cout << "\t" << coordinate << "\n";
        }
    }

    if (outMode == FILE) fclose(stdout);
}


void moveFocusedWindow(std::string rectID) {
    if (!rectMap.count(rectID)) {
        std::cout << "No registered rects found for Rect ID: " << rectID << "\n";
        return;
    }

    HWND hActvWnd = GetForegroundWindow();
    MoveWindow(
        // Window Handle:
        hActvWnd,
        // X:
        rectMap[rectID][0],
        // Y:
        rectMap[rectID][1],
        // Width:
        rectMap[rectID][3] - rectMap[rectID][0],
        // Height:
        rectMap[rectID][2] - rectMap[rectID][1],
        // Repaint:
        true
    );
}


void moveWindow(std::string winID, std::string rectID) {
    MoveWindow(
        winMap[winID],
        rectMap[rectID][0],
        rectMap[rectID][1],
        rectMap[rectID][3] - rectMap[rectID][0],
        rectMap[rectID][2] - rectMap[rectID][1],
        true
    );
}


void setWinID(std::string winID) {
    HWND hActvWnd = GetForegroundWindow();
    winMap[winID] = hActvWnd;
    std::cout << "SET Window ID " << winID << "\n\n";
}


void shiftBuffer(char inChar) {
    for (short i = 0; i < BUFFSIZE; i++) {
        inBuff[i] = inBuff[i + 1];
    }
    inBuff[BUFFSIZE - 1] = inChar;
}


void flushBuffer() {
    for (short i = 0; i < BUFFSIZE; i++) inBuff[i] = '_';
}


void printBuffer() {
    // Debug helper
    for (char c : inBuff) std::cout << c;
    std::cout << ("\n");
}


void readBuffer() {
    std::string winID{ "" };
    std::string rectID{ "" };

    std::cmatch m;
    std::string match;
    //searches
    std::regex reMoveWin{ "(M){1}(T|W\\d+){1}(R\\d+){1}" };
    std::regex reSetRect{ "(SR){1}(\\d+){1}" };
    std::regex reSetWin{ "(SW){1}(\\d+){1}" };
    //matches
    std::regex reGetRect{ "(\\w|\\d)*(GR)" };
    std::regex reFlush{ "(\\d|\\w)*(FLUSH)" };

    if (std::regex_match(inBuff, reFlush)) {
        flushBuffer();
    }

    else if (std::regex_search(inBuff, m, reMoveWin)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";

        // Work backwards over the input
        int i = match.length() - 1;

        while (isdigit(match[i])) {
            rectID.insert(0, 1, match[i]);
            i--;
        }

        //Move over the (R)ect flag:
        i--;

        // Get the window # if one exists:
        if (isdigit(match[i])) {
            while (isdigit(match[i])) {
                winID.insert(0, 1, match[i]);
                i--;
            }
            //Move over the (W) flag:
            i--;
            moveWindow(winID, rectID);
        }
        //Otherwise, it's the focused window:
        else {
            moveFocusedWindow(rectID);
        }
    }

    else if (std::regex_search(inBuff, m, reSetRect)) {
        match = m.str();

        int i = match.length() - 1;
        while (isdigit(match[i])) {
            rectID.insert(0, 1, match[i]);
            i--;
        }
        setRectID(rectID);
    }

    else if (std::regex_search(inBuff, m, reSetWin)) {
        match = m.str();

        int i = match.length() - 1;
        while (isdigit(match[i])) {
            winID.insert(0, 1, match[i]);
            i--;
        }
        setWinID(winID);
    }

    else if (std::regex_match(inBuff, reGetRect)) {
        printRectIDs();
    }


    if (!disableBufferFlush) flushBuffer();
}


void ingressInput() {
    char inChar;
    if ((kbdStruct.vkCode >= 65) && (kbdStruct.vkCode <= 90)) {
        // Letter
        inChar = kbdStruct.vkCode;
    }
    else if ((kbdStruct.vkCode >= 48) && (kbdStruct.vkCode <= 57)) {
        // Numrow
        inChar = kbdStruct.vkCode;
    }
    else if ((kbdStruct.vkCode >= 96) && (kbdStruct.vkCode <= 105)) {
        // Numpad
        inChar = kbdStruct.vkCode - 48; // Offset num0 @ 0
    }
    else if ((kbdStruct.vkCode >= 112) && (kbdStruct.vkCode <= 120)) {
        // Fn 1-9
        inChar = kbdStruct.vkCode - 63; // Offset Fn1 @ 1
    }
    else if (kbdStruct.vkCode == 163) {
        // RCtrl
        readBuffer();
        return;
    }
    else {
        // LCtrl, L/RShift, Space, Backspace, Enter, Tab, etc...
        return;
    }

    shiftBuffer(inChar);
    if (debugBuffer) printBuffer();
}


LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        if (wParam == WM_KEYDOWN) {
            kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
            ingressInput();
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}


void initArgs(int argc, char* argv[]) {
    if (argc < 2) return;

    std::string flag; //g++ throws a warning without it
    for (int i = 1; i < argc; i++) {
        flag = argv[i];
        if (flag == "dbf") {
            disableBufferFlush = true;
        }

        if (flag == "verbose") {
            verbose = true;
        }

        if (flag == "debug") {
            debugBuffer = true;
        }

    }
}


void initAppData() {
    appData = getenv("APPDATA");
    appData.append("\\winDozer");
    if (!std::filesystem::exists(appData)) {
        std::filesystem::create_directory(appData);
    }

    settings = appData;
    settings.append("\\settings.txt");
}


void exitHandler(int SIG) {
    printRectIDs(FILE, settings);
    UnhookWindowsHookEx(hHook);
    exit(SIG);
}


void excludeOthers() {
    std::string lockPath = appData;
    lockPath.append("\\lock");

    HANDLE hFile = CreateFileA(
        lockPath.c_str(),
        (GENERIC_READ | GENERIC_WRITE),
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        exit(0);
    }

}


int main(int argc, char* argv[]) {
    if (
        (hHook = SetWindowsHookExA(
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

        initAppData();
        excludeOthers();
        initArgs(argc, argv);
        loadRectIDs();
        printFigletWelcome();
        flushBuffer();
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
