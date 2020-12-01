#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <map>
#include <vector>
#include <signal.h>
#include "Windows.h"

#define STDOUT 0
#define FILE 1

KBDLLHOOKSTRUCT kbdStruct;
HHOOK hHook;

const short BUFFSIZE{ 7 };
char inBuff[BUFFSIZE];

std::map<std::string, std::vector<int>> rectMap;

char* appData;

void loadRectIDs() {
    std::string rectCoord;
    std::string rectID;
    std::vector<int> rect;
    std::regex reRectID{ "Rect ID\\s{1}(\\d+){1}:{1}" };

    std::string line;
    std::fstream inFile(appData);
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


void printRectIDs(int outMode = STDOUT, char* path = nullptr) {
    if (outMode == FILE) freopen(path, "w", stdout);

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


void moveWindow(std::string winID, std::string rectID) {} // coming soon


void shiftBuffer(char inChar) {
    for (short i = 0; i < BUFFSIZE; i++) {
        inBuff[i] = inBuff[i + 1];
    }
    inBuff[BUFFSIZE - 1] = inChar;
}


void flushBuffer() {
    for (short i = 0; i < BUFFSIZE; i++) inBuff[i] = '_';
}


void readBuffer() {
    std::string winID{ "" };
    std::string rectID{ "" };

    std::cmatch m;
    std::string match;
    std::regex reMoveWin{ "(M){1}(T|W\\d+){1}(R\\d+){1}" };
    std::regex reSetRect{ "(SR){1}(\\d+){1}" };
    std::regex reGetRect{ "(\\w+|\\d+)(GR)" };

    if (std::regex_search(inBuff, m, reMoveWin)) {
        match = m.str();

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
            //Move over the (W)ect flag:
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
    else if (std::regex_match(inBuff, reGetRect)) {
        printRectIDs();
    }

    flushBuffer();
}


void printBuffer() {
    // Helper
    for (char c : inBuff) std::cout << c;
    std::cout << ("\n");
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
    else if ((kbdStruct.vkCode >= 112) && (kbdStruct.vkCode <= 123)) {
        // Fn
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


void initAppData() {
    appData = getenv("APPDATA");
    strcat(appData, "\\winDozer");
    if (!std::filesystem::exists(appData)) {
        std::filesystem::create_directory(appData);
    }
    strcat(appData, "\\settings.txt");
}


void exitHandler(int SIG) {
    printRectIDs(FILE, appData);
    UnhookWindowsHookEx(hHook);
    exit(SIG);
}


void printFigletWelcome() {
    std::cout << ("          _          ___                  \n");
    std::cout << ("__      _(_)_ __    /   \\___ _______ _ __ \n");
    std::cout << ("\\ \\ /\\ / / | '_ \\  / /\\ / _ \\_  / _ \\ '__|\n");
    std::cout << (" \\ V  V /| | | | |/ /_// (_) / /  __/ |   \n");
    std::cout << ("  \\_/\\_/ |_|_| |_/____/ \\___/___\\___|_|\n\n");
    std::cout << ("Press Ctrl+C to exit.\n\n");
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
