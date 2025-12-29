#include <Windows.h>
#include <cctype>
#include <iostream>
#include <regex>
#include <string>
#include "headers/WinDozer.h"

void WinDozer::shiftBuffer(char inChar, bool isBackspace) {
  if (isBackspace) {
    // Backspace: shift right and add placeholder on the left
    for (short i = bufferSize - 1; i > 0; i--) {
      inBuff[i] = inBuff[i - 1];
    }
    inBuff[0] = '_';
  } else {
    // Normal operation: shift left and add new character on the right
    for (short i = 0; i < bufferSize - 1; i++) {
      inBuff[i] = inBuff[i + 1];
    }
    inBuff[bufferSize - 1] = inChar;
  }
}

void WinDozer::initBuffer() {
  if (!bufferSize) {
    bufferSize = 20;  // Fallback default (should be set by initArgs)
  }
  // Only initialize if buffer is empty (idempotent)
  if (inBuff.empty()) {
    inBuff.resize(bufferSize, '_');
  }
}

void WinDozer::flushBuffer() {
  for (short i = 0; i < bufferSize; i++) {
    inBuff[i] = '_';
  }
}

void WinDozer::printBuffer() {
  // Debug helper
  for (char c : inBuff) {
    std::cout << c;
  }
  std::cout << ("\n");
}

// Tokenizer: Convert input string into a sequence of tokens
std::vector<WinDozer::Token> WinDozer::tokenize(const std::string& input) {
  std::vector<Token> tokens;
  size_t position = 0;

  while (position < input.length()) {
    // Skip placeholder characters
    if (input[position] == '_') {
      position++;
      continue;
    }

    // Skip whitespace (though we probably won't have any)
    if (std::isspace(input[position])) {
      position++;
      continue;
    }

    Token token;
    token.position = position;

    // Check if it's a digit (number)
    if (std::isdigit(input[position])) {
      token.type = TokenType::NUMBER;
      while (position < input.length() && std::isdigit(input[position])) {
        token.value += input[position];
        position++;
      }
      tokens.push_back(token);
      continue;
    }

    // Check if it's a letter (command or identifier)
    if (std::isalpha(input[position])) {
      token.type = TokenType::COMMAND;
      // Collect consecutive letters (commands are usually 1-2 letters)
      while (position < input.length() && std::isalpha(input[position])) {
        token.value += input[position];
        position++;
      }
      tokens.push_back(token);
      continue;
    }

    // Unknown character
    token.type = TokenType::UNKNOWN;
    token.value = input[position];
    tokens.push_back(token);
    position++;
  }

  // Add END token
  Token endToken;
  endToken.type = TokenType::END;
  endToken.position = input.length();
  tokens.push_back(endToken);

  return tokens;
}

// Parser: Convert tokens from tokenize() into a Command structure
// This is a simple recursive-descent style parser that walks through tokens
WinDozer::Command WinDozer::parseCommand(const std::vector<Token>& tokens) {
  Command command;

  if (tokens.empty() || tokens[0].type == TokenType::END) {
    return command;  // Empty command
  }

  // Build pattern string for simpler matching
  std::string pattern = "";

  for (const auto& token : tokens) {
    if (token.type == TokenType::END) {
      break;
    }
    if (token.type == TokenType::COMMAND) {
      pattern += token.value;
    } else if (token.type == TokenType::NUMBER) {
      pattern += "N";  // N represents a number in the pattern
    }
  }

  // Since buffer slides left, newest commands are on the right
  // We need to find the RIGHTMOST (most recent) command pattern
  // Use rfind() to find the last occurrence, and track which command appears latest

  struct PatternMatch {
    std::string name;
    size_t position;
    std::string patternStr;
  };

  PatternMatch bestMatch{"", std::string::npos, ""};

  // Check if pattern matches and update bestMatch if it's more recent
  // pattern with the highest (rightmost) position is the most recent and thus the best match
  // When positions are equal, prefer longer patterns (more specific matches)
  auto checkPattern = [&](const std::string& patternStr, const std::string& commandName) {
    size_t position = pattern.rfind(patternStr);
    if (position != std::string::npos) {
      bool shouldUpdate = false;
      if (bestMatch.position == std::string::npos) {
        shouldUpdate = true;
      } else if (position > bestMatch.position) {
        shouldUpdate = true;
      } else if (position == bestMatch.position &&
                 patternStr.length() > bestMatch.patternStr.length()) {
        // Same position, but this pattern is longer (more specific) - prefer it
        shouldUpdate = true;
      }
      if (shouldUpdate) {
        bestMatch.name = commandName;
        bestMatch.position = position;
        bestMatch.patternStr = patternStr;
      }
    }
  };

  // Check all command patterns (order preserved; we still pick the rightmost match)
  struct PatternDef {
    const char* pattern;
    const char* name;
  };

  const std::vector<PatternDef> patternDefs = {
    // With args (longer patterns first to ensure they're preferred):
    {"MWNRN", "MOVE_WINDOW"},     // MW1R1
    {"AWNSN", "ADJUST_WINDOW"},   // AW1S2
    {"ATSN", "ADJUST_THIS"},      // ATS2
    {"SRN", "SET_RECT"},          // SR1
    {"SWN", "SET_WIN"},           // SW1
    {"ERN", "ERASE_RECT"},        // ER1
    {"EWN", "ERASE_WIN"},         // EW1
    {"MTRN", "MOVE_THIS"},        // MTR1
    {"FWN", "FOCUS_WIN"},         // FW1
    {"AWN", "ADJUST_WINDOW"},     // AW1
    {"AT", "ADJUST_THIS"},        // AT
    {"SSN", "SAVE_SNAPSHOT"},     // SS1
    {"RSN", "RESTORE_SNAPSHOT"},  // RS1
                                  // No args:
    {"FLUSH", "FLUSH"},           // FLUSH
    {"HELP", "HELP"},             // HELP
    {"EXIT", "EXIT"},             // EXIT
    {"PT", "PIN_THIS"},           // PT
    {"CT", "CENTER_THIS"},        // CT
    {"GR", "GET_RECTS"},          // GR
    {"GW", "GET_WINS"},           // GW
    {"GS", "GET_SNAPSHOTS"},      // GS
    {"CG", "COPY_GEOMETRY"},      // CG
    {"VG", "PASTE_GEOMETRY"},     // VG
  };

  for (const auto& patternDef : patternDefs) {
    checkPattern(patternDef.pattern, patternDef.name);  // e.g., SRN -> SET_RECT (SR1)
  }

  // If no pattern matched, return empty command
  if (bestMatch.position == std::string::npos) {
    return command;
  }

  command.name = bestMatch.name;

  // Compute command length (for cleanup/backspacing) based on the matched pattern
  int numberSlots = 0;
  for (char c : bestMatch.patternStr) {
    if (c == 'N') {
      numberSlots++;
    }
  }

  int letterLength = static_cast<int>(bestMatch.patternStr.size()) - numberSlots;
  int digitsLength = 0;
  for (size_t i = tokens.size(); i-- > 0 && numberSlots > 0;) {
    if (tokens[i].type == TokenType::NUMBER) {
      digitsLength += static_cast<int>(tokens[i].value.size());
      numberSlots--;
    }
  }
  command.commandLength = letterLength + digitsLength;

  // Argument extraction uses a three-layer defense against stale arguments:
  // 1. Backwards loop: Scan from end -> beginning to get the most recent arguments first
  // 2. Shape verification: Check token types match expected structure (COMMAND followed by NUMBER)
  // 3. Command letter verification: Verify token ends with expected letter (e.g., "R" for SET_RECT)
  //    The tokenizer groups consecutive letters, so "SR50" → Token("SR"), not Token("S")+Token("R")
  //    We check both standalone letter tokens (defensive) and tokens ending with that letter
  //    (common case)
  if (command.name == "SET_RECT") {
    // Look for token ending with "R" (like "SR") followed by number
    // Note: Standalone "R" token is rare (tokenizer groups consecutive letters), but we check for
    // it defensively
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "R" ||
            (tokens[i].value.length() > 0 && tokens[i].value.back() == 'R')) {
          command.args.push_back(tokens[i + 1].value);  // the rectID
          break;
        }
      }
    }
  } else if (command.name == "SET_WIN") {
    // Look for token ending with "W" (like "SW") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "W" ||
            (tokens[i].value.length() > 0 && tokens[i].value.back() == 'W')) {
          command.args.push_back(tokens[i + 1].value);  // the winID
          break;
        }
      }
    }
  } else if (command.name == "ERASE_RECT") {
    // Look for token ending with "R" (like "ER") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "R" ||
            (tokens[i].value.length() > 0 && tokens[i].value.back() == 'R')) {
          command.args.push_back(tokens[i + 1].value);  // the rectID
          break;
        }
      }
    }
  } else if (command.name == "ERASE_WIN") {
    // Look for token ending with "W" (like "EW") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "W" ||
            (tokens[i].value.length() > 0 && tokens[i].value.back() == 'W')) {
          command.args.push_back(tokens[i + 1].value);  // the winID
          break;
        }
      }
    }
  } else if (command.name == "MOVE_THIS") {
    // Look for token ending with "R" (like "MTR") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "R" ||
            (tokens[i].value.length() > 0 && tokens[i].value.back() == 'R')) {
          command.args.push_back(tokens[i + 1].value);  // the rectID
          break;
        }
      }
    }
  } else if (command.name == "MOVE_WINDOW") {
    // Look for "W" followed by number, then "R" followed by number
    std::string winID = "";
    std::string rectID = "";
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 3 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER && tokens[i + 3].type == TokenType::NUMBER) {
        // Verify first command is "W" (or ends with "W" like "MW"), second is "R"
        bool firstIsW =
          tokens[i].value == "W" || (tokens[i].value.length() > 0 && tokens[i].value.back() == 'W');
        bool secondIsR = tokens[i + 2].type == TokenType::COMMAND &&
                         (tokens[i + 2].value == "R" ||
                          (tokens[i + 2].value.length() > 0 && tokens[i + 2].value.back() == 'R'));
        if (firstIsW && secondIsR) {
          winID = tokens[i + 1].value;
          rectID = tokens[i + 3].value;
          break;
        }
      }
    }
    if (!winID.empty() && !rectID.empty()) {
      command.args.push_back(winID);
      command.args.push_back(rectID);
    }
  } else if (command.name == "FOCUS_WIN") {
    // Look for token ending with "W" (like "FW") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "W" ||
            (tokens[i].value.length() > 0 && tokens[i].value.back() == 'W')) {
          command.args.push_back(tokens[i + 1].value);  // the winID
          break;
        }
      }
    }
  } else if (command.name == "ADJUST_WINDOW") {
    std::string winID = "";
    std::string step = "";

    // Check if pattern includes step (AWNSN vs AWN)
    bool hasStep = bestMatch.patternStr.find("SN") != std::string::npos;

    for (size_t i = tokens.size(); i-- > 0;) {
      if (hasStep) {
        // With step: AW + 5 + S + 10
        if (tokens[i].type == TokenType::COMMAND && i + 3 < tokens.size() &&
            tokens[i + 1].type == TokenType::NUMBER && tokens[i + 3].type == TokenType::NUMBER) {
          // Verify first is "W" (or ends with "W" like "AW"), second ends with "S"
          bool firstIsW = tokens[i].value == "W" ||
                          (tokens[i].value.length() > 0 && tokens[i].value.back() == 'W');
          bool secondIsS = tokens[i + 2].type == TokenType::COMMAND &&
                           !tokens[i + 2].value.empty() && tokens[i + 2].value.back() == 'S';
          if (firstIsW && secondIsS) {
            winID = tokens[i + 1].value;
            step = tokens[i + 3].value;
            break;
          }
        }
      } else {
        // Without step: AW + 5
        if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
            tokens[i + 1].type == TokenType::NUMBER) {
          // Verify this is "W" (or ends with "W" like "AW")
          if (tokens[i].value == "W" ||
              (tokens[i].value.length() > 0 && tokens[i].value.back() == 'W')) {
            winID = tokens[i + 1].value;
            break;
          }
        }
      }
    }

    if (!winID.empty()) {
      command.args.push_back(winID);
    }
    if (!step.empty()) {
      command.args.push_back(step);
    }
  } else if (command.name == "ADJUST_THIS") {
    // Only extract step if pattern includes "SN" (e.g., "ATSN" not "AT")
    if (bestMatch.patternStr.find("SN") != std::string::npos) {
      // Look for a command token ending with "S" followed by number
      // Note: Tokenizer groups consecutive letters, so "ATS50" becomes Token("ATS") + Token("50")
      for (size_t i = tokens.size(); i-- > 0;) {
        if (tokens[i].type == TokenType::COMMAND && !tokens[i].value.empty() &&
            tokens[i].value.back() == 'S' && i + 1 < tokens.size() &&
            tokens[i + 1].type == TokenType::NUMBER) {
          command.args.push_back(tokens[i + 1].value);  // the step
          break;
        }
      }
    }
    // If pattern is just "AT" (no step), don't extract any args - use default step of 1
  } else if (command.name == "SAVE_SNAPSHOT") {
    // Look for "SS" (or token ending with "SS") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "SS" ||
            (tokens[i].value.length() >= 2 &&
             tokens[i].value.substr(tokens[i].value.length() - 2) == "SS")) {
          command.args.push_back(tokens[i + 1].value);  // the snapshotID
          break;
        }
      }
    }
  } else if (command.name == "RESTORE_SNAPSHOT") {
    // Look for "RS" (or token ending with "RS") followed by number
    for (size_t i = tokens.size(); i-- > 0;) {
      if (tokens[i].type == TokenType::COMMAND && i + 1 < tokens.size() &&
          tokens[i + 1].type == TokenType::NUMBER) {
        if (tokens[i].value == "RS" ||
            (tokens[i].value.length() >= 2 &&
             tokens[i].value.substr(tokens[i].value.length() - 2) == "RS")) {
          command.args.push_back(tokens[i + 1].value);  // the snapshotID
          break;
        }
      }
    }
  }

  return command;
}

// Dispatcher: Execute the result from parseCommand()
bool WinDozer::executeCommand(const Command& command) {
  if (command.name.empty()) {
    return false;
  }

  bool cleanedUp = false;

  if (command.name == "SET_RECT") {
    if (!command.args.empty()) {
      setRectID(command.args[0]);
    }
  } else if (command.name == "SET_WIN") {
    if (!command.args.empty()) {
      setWinID(command.args[0]);
    }
  } else if (command.name == "ERASE_RECT") {
    if (!command.args.empty()) {
      eraseRectID(command.args[0]);
    }
  } else if (command.name == "ERASE_WIN") {
    if (!command.args.empty()) {
      eraseWinID(command.args[0]);
    }
  } else if (command.name == "MOVE_THIS") {
    if (!command.args.empty()) {
      moveWindow(command.args[0]);
    }
  } else if (command.name == "MOVE_WINDOW") {
    if (command.args.size() >= 2) {
      moveWindow(command.args[0], command.args[1]);
    }
  } else if (command.name == "FOCUS_WIN") {
    if (!command.args.empty()) {
      // Perform cleanUp in the relevant window before focusing:
      std::string currentBuffer(inBuff.begin(), inBuff.end());
      if (currentBuffer != lastCleanedBuffer) {
        cleanUp(std::string(static_cast<size_t>(command.commandLength), '_'));
        lastCleanedBuffer = currentBuffer;
      }
      cleanedUp = true;
      focusWindow(command.args[0]);
    }
  } else if (command.name == "ADJUST_WINDOW") {
    // command.args[0] = winID, command.args[1] = step (optional)
    if (!command.args.empty()) {
      int step = command.args.size() > 1 ? std::stoi(command.args[1]) : 1;
      enterAdjustWindowMode(command.args[0], step);
    }
  } else if (command.name == "ADJUST_THIS") {
    int step = command.args.empty() ? 1 : std::stoi(command.args[0]);
    enterAdjustWindowMode("", step);
  } else if (command.name == "SAVE_SNAPSHOT") {
    if (!command.args.empty()) {
      saveLayoutSnapshot(command.args[0]);
    }
  } else if (command.name == "RESTORE_SNAPSHOT") {
    if (!command.args.empty()) {
      restoreLayoutSnapshot(command.args[0]);
    }
  } else if (command.name == "PIN_THIS") {
    pinThis();
  } else if (command.name == "CENTER_THIS") {
    centerThis();
  } else if (command.name == "GET_RECTS") {
    printRects();
  } else if (command.name == "GET_WINS") {
    printWinIDs();
  } else if (command.name == "GET_SNAPSHOTS") {
    printSnapshots();
  } else if (command.name == "COPY_GEOMETRY") {
    copyGeometry();
  } else if (command.name == "PASTE_GEOMETRY") {
    pasteGeometry();
  } else if (command.name == "FLUSH") {
    flushBuffer();
  } else if (command.name == "HELP") {
    printHelp();
  } else if (command.name == "EXIT") {
    // Clean up now, because we don't reach that far after this call to exitWinDozer()
    cleanUp(std::string(static_cast<size_t>(command.commandLength), '_'));
    exitWinDozer();
  }

  else {
    return false;  // Unknown command
  }

  if (!cleanedUp) {
    // Convert current buffer to string for comparison
    std::string currentBuffer(inBuff.begin(), inBuff.end());

    // Only cleanup if buffer content changed (user typed something new)
    // This prevents duplicate cleanup when repeating commands with dbf enabled
    if (currentBuffer != lastCleanedBuffer) {
      cleanUp(std::string(static_cast<size_t>(command.commandLength), '_'));
      lastCleanedBuffer = currentBuffer;
    }
  }

  if (!disableBufferFlush) {
    flushBuffer();
    // Reset tracking when buffer flushes (new command cycle)
    lastCleanedBuffer.clear();
  }

  return true;
}

// Interpreter-based command parsing (alternative to regex)
// This uses a tokenizer + parser + dispatcher approach
//
// Buffer: "MW5R10"
//   ↓
// Tokenizer: [Token(COMMAND, "MW"), Token(NUMBER, "5"), Token(COMMAND, "R"), Token(NUMBER, "10")]
//   ↓
// Parser: Command{name: "MOVE_WINDOW", args: ["5", "10"]}
//   ↓
// Dispatcher: moveWindow("5", "10")
void WinDozer::interpretBuffer() {
  std::string buffString(inBuff.begin(), inBuff.end());

  // 1: Tokenize the buffer
  std::vector<Token> tokens = tokenize(buffString);

  // 2: Parse tokens into a command
  Command command = parseCommand(tokens);

  // 3: Execute the command
  if (!command.name.empty()) {
    if (debug) {
      std::cout << "Command: " << command.name;
      for (const auto& argument : command.args) {
        std::cout << " " << argument;
      }
      std::cout << "\n";
    }
    executeCommand(command);
  }
}

void WinDozer::ingressInput() {
  char inChar;
  bool isBackspace = false;

  if (!adjusting) {
    if ((kbdStruct.vkCode >= VK_A) && (kbdStruct.vkCode <= VK_Z)) {
      // Letter
      inChar = kbdStruct.vkCode;
    } else if ((kbdStruct.vkCode >= VK_0) && (kbdStruct.vkCode <= VK_9)) {
      // Numrow
      inChar = kbdStruct.vkCode;
    } else if ((kbdStruct.vkCode >= VK_NUMPAD0) && (kbdStruct.vkCode <= VK_NUMPAD9)) {
      // Numpad
      inChar = kbdStruct.vkCode - NUMPAD_TO_NUMROW_OFFSET;
    } else if ((kbdStruct.vkCode >= VK_F1) && (kbdStruct.vkCode <= VK_F9)) {
      // Function keys F1-F9
      inChar = kbdStruct.vkCode - FUNCTION_KEY_TO_NUMROW_OFFSET;
    } else if (kbdStruct.vkCode == VK_BACK) {
      if (isPerformingCleanup) {  // Only process non-synthesized backspaces
        return;
      }
      inChar = 0;  // Dummy value, won't be used
      isBackspace = true;
    } else if (kbdStruct.vkCode == SUBMIT) {
      interpretBuffer();
      return;
    } else {
      // Space, Enter, Tab, etc...
      return;
    }
  } else {
    if ((kbdStruct.vkCode >= VK_LEFT) && (kbdStruct.vkCode <= VK_DOWN)) {
      // Arrow key - check if modifier key is held down (for resize mode)
      bool isResizeMode = (GetAsyncKeyState(MODIFIER) & KEY_STATE_PRESSED) != 0;
      adjustWindow(kbdStruct.vkCode, isResizeMode);
      return;

    } else if (kbdStruct.vkCode == SUBMIT) {
      exitAdjustWindowMode();
      return;
    } else {
      return;
    }
  }

  shiftBuffer(inChar, isBackspace);
  if (debug) {
    printBuffer();
  }
}
