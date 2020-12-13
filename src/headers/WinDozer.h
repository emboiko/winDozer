#ifndef WINDOZER_H
#define WINDOZER_H


#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <map>
#include <vector>
#include <Windows.h>


struct WinDozer {
    KBDLLHOOKSTRUCT kbdStruct;
    HHOOK hKbdHook;
    HWINEVENTHOOK hWinEventHook;

    static const short STDOUT{ 0 };
    static const short FILE{ 1 };

    static const short BUFFSIZE{ 7 };
    char inBuff[BUFFSIZE];

    std::map<std::string, std::vector<int>> rectMap;
    std::map<std::string, HWND> winMap;

    std::string appData;
    std::string settings;

    bool disableBufferFlush;
    bool verbose;
    bool debugBuffer;

    void printFigletWelcome();

    void printHelp();

    void loadRectIDs();

    void setRectID(std::string rectID);

    void eraseRectID(std::string rectID);

    void printRectIDs(int outMode, std::string path);

    void printWinIDs();

    void focusWindow(std::string winID);

    void moveWindow(std::string rectID);

    void moveWindow(std::string winID, std::string rectID);

    void setWinID(std::string winID);

    void eraseWinID(std::string winID);

    void shiftBuffer(char inChar);

    void flushBuffer();

    void printBuffer();

    void readBuffer();

    void ingressInput();

    void initArgs(int argc, char* argv[]);

    void initAppData();

    void excludeOthers();

    int getSuffixID(std::string match, std::string& idString);

    int getSuffixID(std::string match, std::string& idString, int i);

    bool validWindow(HWND hWnd);

    std::string registered(HWND hWnd);
};


#endif // WINDOZER_H
