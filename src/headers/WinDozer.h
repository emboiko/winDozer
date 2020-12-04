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
    HHOOK hHook;

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

    void printRectIDs(int outMode, std::string path);

    void printWinIDs();

    void focusWindow(std::string winID);

    void moveFocusedWindow(std::string rectID);

    void moveWindow(std::string winID, std::string rectID);

    void setWinID(std::string winID);

    void shiftBuffer(char inChar);

    void flushBuffer();

    void printBuffer();

    void readBuffer();

    void ingressInput();

    void initArgs(int argc, char* argv[]);

    void initAppData();

    void excludeOthers();
};


#endif // WINDOZER_H
