#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <map>
#include <vector>
#include <signal.h>
#include <Windows.h>
#include "headers/WinDozer.h"


void WinDozer::printFigletWelcome() {
    std::cout
        << ("          _          ___                  \n")
        << ("__      _(_)_ __    /   \\___ _______ _ __ \n")
        << ("\\ \\ /\\ / / | '_ \\  / /\\ / _ \\_  / _ \\ '__|\n")
        << (" \\ V  V /| | | | |/ /_// (_) / /  __/ |   \n")
        << ("  \\_/\\_/ |_|_| |_/____/ \\___/___\\___|_|\n\n");
}


void WinDozer::printHelp() {
    printFigletWelcome();
    std::cout
        << ("Press Ctrl+C to exit.\n\n")
        << ("https://github.com/emboiko/winDozer\n\n")
        << ("Flags:\n\n")
        << ("\tdbf\t\t\t\t Disable Buffer Flush\n")
        << ("\tverbose\t\t\t\t Extra console feedback\n")
        << ("\tdebug\t\t\t\t Flood stdout with esoteric logging\n")
        << ("\n")
        << ("Syntax:\n\n")
        << ("\tSR{Rect ID}\t\t\t Set Rect ID\n")
        << ("\tSW{Window ID}\t\t\t Set Window ID\n")
        << ("\tER{Rect ID}\t\t\t Erase Rect ID\n")
        << ("\tEW{Window ID}\t\t\t Erase Window ID\n")
        << ("\tMTR{Rect ID}\t\t\t Move This [window] to Rect\n")
        << ("\tMW{Window ID}R{Rect ID}\t\t Move Window to Rect\n")
        << ("\tFW{Window ID}\t\t\t Focus registered Window by ID\n")
        << ("\tAW{Window ID}\t\t\t Adjust Window by ID\n")
        << ("\tAT\t\t\t\t Adjust This [window]\n")
        << ("\tGR\t\t\t\t Get/Print all registered Rects\n")
        << ("\tGW\t\t\t\t Get/Print all registered Windows\n")
        << ("\tFLUSH\t\t\t\t Flush Buffer\n")
        << ("\tHELP\t\t\t\t Print this dialog\n")
        << ("\t<RCtrl>\t\t\t\t Submit\n")
        << ("\n");
}


void WinDozer::loadRectIDs() {
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


void WinDozer::setRectID(std::string rectID) {
    std::vector<int> rect;

    RECT winRect;
    HWND hActvWnd = GetForegroundWindow();
    if (!validWindow(hActvWnd)) return;

    GetWindowRect(hActvWnd, &winRect);

    rect.push_back(winRect.left);
    rect.push_back(winRect.top);
    rect.push_back(winRect.bottom);
    rect.push_back(winRect.right);

    rectMap[rectID] = rect;

    std::cout << "SET RectID " << rectID << "\n\n";
}


void WinDozer::eraseRectID(std::string rectID) {
    if (!rectMap.erase(rectID)) {
        std::cout << "No registered rects found for Rect ID: "
            << rectID << "\n";
    }
    else {
        std::cout << "ERASE RectID " << rectID << "\n\n";
    };
}


void WinDozer::printRectIDs(int outMode = STDOUT, std::string path = "") {
    if (rectMap.empty()) {
        std::cout << "No registered Rect ID(s) found\n";
        return;
    }

    if (outMode == FILE) freopen(path.c_str(), "w+", stdout);

    for (auto it = rectMap.begin(); it != rectMap.end(); it++) {
        std::string rectID = it->first;
        std::cout << "Rect ID " << rectID << ":\n";
        for (int coordinate : rectMap[rectID]) {
            std::cout << "\t" << coordinate << "\n";
        }
    }

    if (outMode == FILE) fclose(stdout);

    std::cout << "\n";
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
            << "\t\t" << classText << "\n\n";
    }
}


void WinDozer::enterAdjustWindow(std::string winID) {
    HWND hwnd;
    if (winID.empty()) {
        hwnd = GetForegroundWindow();
        if (!validWindow(hwnd)) {
            adjusting = false;
            return;
        }

        hAdjustedWindow = hwnd;

    }
    else {
        if (!winMap.count(winID)) {
            std::cout << "No registered windows found for Window ID: "
                << winID << "\n";

            adjusting = false;
            return;
        }

        hAdjustedWindow = winMap[winID];
    }

    adjusting = true;
    char winText[MAX_PATH];
    GetWindowTextA(hAdjustedWindow, winText, MAX_PATH);
    char classText[MAX_PATH];
    GetClassNameA(hAdjustedWindow, classText, sizeof(classText));
    std::cout << "Entering adjustment: " << classText << "\n"
        << winText << "\n\n";
}


void WinDozer::adjustWindow(DWORD vkCode) {
    //Raise the window without necessarily focusing it
    if (IsIconic(hAdjustedWindow)) ShowWindow(hAdjustedWindow, SW_RESTORE);

    RECT winRect;
    GetWindowRect(hAdjustedWindow, &winRect);

    int x = winRect.left;
    int y = winRect.top;
    switch (vkCode) {
    case 37:
        // Left
        x--;
        break;
    case 38:
        // Up
        y--;
        break;
    case 39:
        // Right
        x++;
        break;
    case 40:
        // Down
        y++;
        break;
    }

    MoveWindow(
        hAdjustedWindow,
        x,
        y,
        winRect.right - winRect.left,
        winRect.bottom - winRect.top,
        true
    );
}


void WinDozer::focusWindow(std::string winID) {
    if (!winMap.count(winID)) {
        std::cout << "No registered windows found for Window ID: "
            << winID << "\n";
        return;
    }

    // Restore the window if neccesary:
    if (IsIconic(winMap[winID])) ShowWindow(winMap[winID], SW_RESTORE);
    // Perform a magic ritual to please the Windows Gods:
    keybd_event(0, 0, 0, 0);
    // Focus the window:
    SetForegroundWindow(winMap[winID]);
}


void WinDozer::moveWindow(std::string rectID) {
    if (!rectMap.count(rectID)) {
        std::cout << "No registered rects found for Rect ID: "
            << rectID << "\n";
        return;
    }

    HWND hActvWnd = GetForegroundWindow();
    if (!validWindow(hActvWnd)) return;

    ShowWindow(hActvWnd, SW_RESTORE); // Window won't move if it's maximized
    MoveWindow(
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


void WinDozer::moveWindow(std::string winID, std::string rectID) {
    if (!winMap.count(winID)) {
        std::cout << "No registered windows found for Window ID: "
            << winID << "\n";
        return;
    }
    if (!rectMap.count(rectID)) {
        std::cout << "No registered rects found for Rect ID: "
            << rectID << "\n";
        return;
    }

    ShowWindow(winMap[winID], SW_RESTORE); // Restore the window if it's minimized
    MoveWindow(
        winMap[winID],
        rectMap[rectID][0],
        rectMap[rectID][1],
        rectMap[rectID][3] - rectMap[rectID][0],
        rectMap[rectID][2] - rectMap[rectID][1],
        TRUE
    );
}


void WinDozer::setWinID(std::string winID) {
    HWND hActvWnd = GetForegroundWindow();
    if (!validWindow(hActvWnd)) return;

    winMap[winID] = hActvWnd;
    std::cout << "SET Window ID " << winID << "\n\n";
}


void WinDozer::eraseWinID(std::string winID) {
    if (!winMap.erase(winID)) {
        std::cout << "No registered windows found for Window ID: "
            << winID << "\n";
    }
    else {
        std::cout << "ERASE Window ID: " << winID << "\n\n";
    };
}


void WinDozer::shiftBuffer(char inChar) {
    for (short i = 0; i < BUFFSIZE; i++) {
        inBuff[i] = inBuff[i + 1];
    }
    inBuff[BUFFSIZE - 1] = inChar;
}


void WinDozer::flushBuffer() {
    for (short i = 0; i < BUFFSIZE; i++) inBuff[i] = '_';
}


void WinDozer::printBuffer() {
    // Debug helper
    for (char c : inBuff) std::cout << c;
    std::cout << ("\n");
}


void WinDozer::readBuffer() {
    std::string winID{ "" };
    std::string rectID{ "" };

    std::cmatch m;
    std::string match;
    //searches
    std::regex reMoveWin{ "(M){1}(T|W\\d+){1}(R\\d+){1}" };
    std::regex reSetRect{ "(SR){1}(\\d+){1}" };
    std::regex reSetWin{ "(SW){1}(\\d+){1}" };
    std::regex reEraseRect{ "(ER){1}(\\d+){1}" };
    std::regex reEraseWin{ "(EW){1}(\\d+){1}" };
    std::regex reFocusWin{ "(FW){1}(\\d+){1}" };
    std::regex reAdjustWin{ "(A){1}(T|W\\d+){1}" };
    //matches
    std::regex reGetRects{ "(\\w|\\d)*(GR)" };
    std::regex reGetWins{ "(\\w|\\d)*(GW)" };
    std::regex reFlush{ "(\\d|\\w)*(FLUSH)" };
    std::regex reHelp{ "(\\d|\\w)*(HELP)" };

    if (std::regex_match(inBuff, reFlush)) {
        flushBuffer();
    }

    else if (std::regex_search(inBuff, m, reMoveWin)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";

        //Start at the back
        int i = getSuffixID(match, rectID);

        //Move over the (R)ect flag
        i--;
        // Get the (W)indow # if one exists:
        if (isdigit(match[i])) {
            getSuffixID(match, winID, i);
            moveWindow(winID, rectID);
        }
        //Otherwise, it's the focused window:
        else {
            moveWindow(rectID);
        }
    }

    else if (std::regex_search(inBuff, m, reSetRect)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";
        getSuffixID(match, rectID);
        setRectID(rectID);
    }

    else if (std::regex_search(inBuff, m, reSetWin)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";
        getSuffixID(match, winID);
        setWinID(winID);
    }

    else if (std::regex_search(inBuff, m, reEraseRect)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";
        getSuffixID(match, rectID);
        eraseRectID(rectID);
    }

    else if (std::regex_search(inBuff, m, reEraseWin)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";
        getSuffixID(match, winID);
        eraseWinID(winID);
    }

    else if (std::regex_search(inBuff, m, reFocusWin)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";
        getSuffixID(match, winID);
        focusWindow(winID);
    }

    else if (std::regex_search(inBuff, m, reAdjustWin)) {
        match = m.str();
        if (verbose) std::cout << match << "\n";
        getSuffixID(match, winID);
        enterAdjustWindow(winID);
    }

    else if (std::regex_match(inBuff, reGetRects)) {
        if (verbose) std::cout << match << "\n";
        printRectIDs();
    }

    else if (std::regex_match(inBuff, reGetWins)) {
        if (verbose) std::cout << match << "\n";
        printWinIDs();
    }

    else if (std::regex_match(inBuff, reHelp)) {
        if (verbose) std::cout << match << "\n";
        printHelp();
    }


    if (!disableBufferFlush) flushBuffer();
}


void WinDozer::ingressInput() {
    char inChar;

    if (!adjusting) {
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
    }
    else {
        if ((kbdStruct.vkCode >= 37) && (kbdStruct.vkCode <= 40)) {
            // Arrow key
            adjustWindow(kbdStruct.vkCode);
            return;

        }
        else if (kbdStruct.vkCode == 163) {
            std::cout << "Exit Adjustment.\n";
            // RCtrl
            adjusting = false;
            hAdjustedWindow = NULL;
            return;
        }
        else {
            // Anything else
            return;
        }
    }

    shiftBuffer(inChar);
    if (debugBuffer) printBuffer();
}


void WinDozer::initArgs(int argc, char* argv[]) {
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


void WinDozer::initAppData() {
    appData = getenv("APPDATA");
    appData.append("\\winDozer");
    if (!std::filesystem::exists(appData)) {
        std::filesystem::create_directory(appData);
    }

    settings = appData;
    settings.append("\\settings.txt");
}


void WinDozer::excludeOthers() {
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


int WinDozer::getSuffixID(std::string match, std::string& idString) {
    int i = match.length() - 1;
    while (isdigit(match[i])) {
        idString.insert(0, 1, match[i]);
        i--;
    }
    return i;
}


int WinDozer::getSuffixID(std::string match, std::string& idString, int i) {
    while (isdigit(match[i])) {
        idString.insert(0, 1, match[i]);
        i--;
    }
    return i;
}


bool WinDozer::validWindow(HWND hWnd) {
    char classText[MAX_PATH];
    GetClassNameA(hWnd, classText, sizeof(classText));
    std::string className = classText;

    if (verbose) { std::cout << "Window class: " << className << "\n"; }

    if (
        className == "Windows.UI.Core.CoreWindow" || // The start menu
        className == "Shell_TrayWnd" || // The system tray
        className == "Progman" || // The desktop itself / child context menu
        className == "Program Manager" // The desktop itself / child context menu
    ) {
        std::cout << "Invalid Window Class: " << className << "\n";
        return false;
    }

    return true;
}


std::string WinDozer::registered(HWND hWnd) {
    if (winMap.empty()) return "";

    std::string winID;
    for (auto it = winMap.begin(); it != winMap.end(); it++) {
        winID = it->first;
        if (winMap[winID] == hWnd) return winID;
    }

    return "";
}
