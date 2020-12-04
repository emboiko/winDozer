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
        << ("\tMTR{Rect ID}\t\t\t Move This [window] to Rect\n")
        << ("\tSW{Window ID}\t\t\t Set Window ID\n")
        << ("\tMW{Window ID}R{Rect ID}\t\t Move Window to Rect\n")
        << ("\tFW{Window ID}\t\t\t Focus registered Window by ID\n")
        << ("\tGR\t\t\t\t Get/Print all registered Rects\n")
        << ("\tGW\t\t\t\t Get/Print all registered Windows\n")
        << ("\tFLUSH\t\t\t\t Flush Buffer\n")
        << ("\tHELP\t\t\t\t Print this dialog\n")
        << ("\n");
    ;
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
    GetWindowRect(hActvWnd, &winRect);

    rect.push_back(winRect.left);
    rect.push_back(winRect.top);
    rect.push_back(winRect.bottom);
    rect.push_back(winRect.right);

    rectMap[rectID] = rect;

    std::cout << "SET RectID " << rectID << "\n\n";
}


void WinDozer::printRectIDs(int outMode = STDOUT, std::string path = "") {
    if (rectMap.empty()) {
        std::cout << "No registered Rect ID(s) found\n";
        return;
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

    std::cout << "\n";
}


void WinDozer::printWinIDs() {
    if (winMap.empty()) {
        std::cout << "No registered Window ID(s) found\n";
        return;
    }

    char winText[MAX_PATH];
    for (auto it = winMap.begin(); it != winMap.end(); it++) {
        std::string winID = it->first;
        GetWindowTextA(winMap[winID], winText, MAX_PATH);
        std::cout << "Window ID " << winID << ":\t" << winText << "\n\n";
    }
}


void WinDozer::focusWindow(std::string winID) {
    if (!winMap.count(winID)) {
        std::cout << "No registered windows found for Window ID: "
            << winID << "\n";
        return;
    }
    ShowWindow(winMap[winID], SW_RESTORE); //Restore the window if it's minimized
    SwitchToThisWindow(winMap[winID], FALSE); //Focus the window

    // Remove the flash
    FLASHWINFO flashInfo;
    flashInfo.cbSize = sizeof(FLASHWINFO);
    flashInfo.hwnd = winMap[winID];
    flashInfo.dwFlags = FLASHW_STOP;
    flashInfo.uCount = 0;
    flashInfo.dwTimeout = 0;
    FlashWindowEx(&flashInfo);
}


void WinDozer::moveFocusedWindow(std::string rectID) {
    if (!rectMap.count(rectID)) {
        std::cout << "No registered rects found for Rect ID: "
            << rectID << "\n";
        return;
    }

    HWND hActvWnd = GetForegroundWindow();
    if (!validWindow(hActvWnd)) return;

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


void WinDozer::moveWindow(std::string winID, std::string rectID) {
    if (!winMap.count(winID)) {
        std::cout << "No registered windows found for Window ID: "
            << winID << "\n";
        return;
    }

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
    std::regex reFocusWin{ "(FW){1}(\\d+){1}" };
    //matches
    std::regex reGetRect{ "(\\w|\\d)*(GR)" };
    std::regex reGetWins{ "(\\w|\\d)*(GW)" };
    std::regex reFlush{ "(\\d|\\w)*(FLUSH)" };
    std::regex reHelp{ "(\\d|\\w)*(HELP)" };

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

    else if (std::regex_search(inBuff, m, reFocusWin)) {
        match = m.str();

        int i = match.length() - 1;
        while (isdigit(match[i])) {
            winID.insert(0, 1, match[i]);
            i--;
        }
        focusWindow(winID);
    }

    else if (std::regex_match(inBuff, reGetRect)) {
        printRectIDs();
    }

    else if (std::regex_match(inBuff, reGetWins)) {
        printWinIDs();
    }

    else if (std::regex_match(inBuff, reHelp)) {
        printHelp();
    }


    if (!disableBufferFlush) flushBuffer();
}


void WinDozer::ingressInput() {
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


bool WinDozer::validWindow(HWND hWnd) {
    char classBuffer[MAX_PATH];
    GetClassNameA(hWnd, classBuffer, sizeof(classBuffer));
    std::string className = classBuffer;

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
