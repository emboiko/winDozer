#include <Windows.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include "headers/WinDozer.h"

// Helper function to parse comma-separated coordinates
std::vector<int> parseRectCoords(const std::string& coordinatesString) {
  std::vector<int> coordinates;
  std::stringstream stringStream(coordinatesString);
  std::string item;

  while (std::getline(stringStream, item, ',')) {
    try {
      coordinates.push_back(std::stoi(item));
    } catch (const std::exception& exception) {
      // Skip invalid coordinates
      continue;
    }
  }

  return coordinates;
}

void WinDozer::loadRectIDs() {
  // Load rect IDs from INI format settings file. Uses a two-pass approach: this is a common pattern
  // when working with Windows INI files because GetPrivateProfileStringA requires knowing the key
  // name - there's no API to enumerate all keys in a section. So we first discover the keys, then
  // read their values.

  if (!std::filesystem::exists(settingsPath)) {
    return;  // No settings file yet - nothing to load
  }

  std::ifstream inFile(settingsPath);
  if (!inFile.is_open()) {
    return;  // Couldn't open file - skip loading
  }

  std::string line;
  bool inRectsSection = false;
  std::set<std::string> rectIDs;

  // PASS 1: Discover all rect ID keys by reading the fileas text We scan through the file to find
  // all keys in the [Rects] section
  while (std::getline(inFile, line)) {
    // Trim leading/trailing whitespace from the line
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    // Check if we've entered the [Rects] section
    if (line == "[Rects]") {
      inRectsSection = true;
      continue;
    }
    // Check if we've left the section (any other section
    // header)
    if (line[0] == '[') {
      inRectsSection = false;
      continue;
    }

    // If we're in the Rects section and this line has a key=value format
    if (inRectsSection && !line.empty() && line.find('=') != std::string::npos) {
      // Extract the key (everything before the '=')
      size_t equalsPosition = line.find('=');
      std::string key = line.substr(0, equalsPosition);
      // Trim whitespace from the key
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      // Store the rect ID for later use
      rectIDs.insert(key);
    }
  }
  inFile.close();

  // PASS 2: Load values using Windows INI API. Now that we know all the rect IDs, we use
  // GetPrivateProfileStringA to read each value. This is more robust than parsing ourselves because
  // the Windows API handles INI file edge cases (comments, whitespace, etc.) correctly.
  char buffer[512];
  for (const std::string& rectID : rectIDs) {
    DWORD len = GetPrivateProfileStringA("Rects", rectID.c_str(), "", buffer, sizeof(buffer),
                                         settingsPath.c_str());
    if (len > 0) {
      // Parse the comma-separated coordinate string (e.g.,
      // "100,200,300,400")
      std::string coordinatesString(buffer);
      std::vector<int> coordinates = parseRectCoords(coordinatesString);
      // Only store if we got exactly 4 coordinates (left,
      // top, bottom, right)
      if (coordinates.size() == 4) {
        rectMap[rectID] = coordinates;
      }
    }
  }
}

bool WinDozer::saveRectIDs(std::string path) {
  // Clear existing rects section (Windows API doesn't have a delete section function, so we
  // overwrite) Save all current rects
  for (const auto& [rectID, coordinates] : rectMap) {
    if (coordinates.size() == 4) {
      std::string coordinatesString =
        std::to_string(coordinates[0]) + "," + std::to_string(coordinates[1]) + "," +
        std::to_string(coordinates[2]) + "," + std::to_string(coordinates[3]);
      WritePrivateProfileStringA("Rects", rectID.c_str(), coordinatesString.c_str(), path.c_str());
    }
  }

  return true;
}

bool WinDozer::readKeyboardRepeatRate() {
  HKEY registryKey;
  LONG result =
    RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Keyboard", 0, KEY_READ, &registryKey);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Unable to open registry key: "
                 "Control Panel\\Keyboard\n";
    return false;
  }

  // Read the KeyboardSpeed value (REG_SZ string type)
  DWORD bufferSize = 256;  // Reasonable size for a numeric string
  char buffer[256] = {0};

  result = RegQueryValueExA(registryKey, "KeyboardSpeed", NULL, NULL,
                            reinterpret_cast<LPBYTE>(buffer), &bufferSize);

  RegCloseKey(registryKey);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Failed to read KeyboardSpeed from "
                 "registry\n";
    return false;
  }

  // Convert the string value to integer
  try {
    KBD_REPEAT_RATE = std::stoi(buffer);
    return true;
  } catch (const std::exception& exception) {
    std::cerr << "Error: Invalid KeyboardSpeed value in registry: " << buffer << "\n";
    return false;
  }
}

bool WinDozer::initArgs(int argc, char* argv[]) {
  // Set default values
  SUBMIT = VK_RCONTROL;
  MODIFIER = VK_LCONTROL;
  bufferSize = 20;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    std::string flag = argv[i];

    // Boolean flags (no arguments)
    if (flag == "dbf") {
      disableBufferFlush = true;
    } else if (flag == "verbose") {
      verbose = true;
    } else if (flag == "debug") {
      debug = true;
    } else if (flag == "cleanup") {
      cleanup = true;
      // Read keyboard repeat rate from registry for cleanup
      // timing
      if (!readKeyboardRepeatRate()) {
        return false;
      }
    }
    // Virtual key code submit flag: vks{number}
    // Example: vks162 (for Left Control)
    else if (flag.length() > 3 && flag.substr(0, 3) == "vks") {
      std::string value = flag.substr(3);

      try {
        int virtualKeyNumber = std::stoi(value);
        if (virtualKeyNumber <= 0 || virtualKeyNumber > 255) {
          std::cerr << "Error: Invalid virtual key code: " << value << " (must be 1-255)\n";
          return false;
        }
        SUBMIT = static_cast<DWORD>(virtualKeyNumber);
      } catch (const std::exception& exception) {
        std::cerr << "Error: Invalid virtual key code format: " << value << "\n";
        return false;
      }
    }
    // Virtual key code modifier flag: vkm{number}
    // Example: vkm162 (for Left Control, default)
    else if (flag.length() > 3 && flag.substr(0, 3) == "vkm") {
      std::string value = flag.substr(3);

      try {
        int virtualKeyNumber = std::stoi(value);
        if (virtualKeyNumber <= 0 || virtualKeyNumber > 255) {
          std::cerr << "Error: Invalid virtual key code: " << value << " (must be 1-255)\n";
          return false;
        }
        MODIFIER = static_cast<DWORD>(virtualKeyNumber);
      } catch (const std::exception& exception) {
        std::cerr << "Error: Invalid virtual key code format: " << value << "\n";
        return false;
      }
    }
    // Buffer size flag: bs{number}
    // Example: bs10 (for 10 character buffer)
    else if (flag.length() > 2 && flag.substr(0, 2) == "bs") {
      std::string value = flag.substr(2);

      try {
        int bufferSizeValue = std::stoi(value);
        if (bufferSizeValue < 7) {
          std::cerr << "Error: Invalid buffer size: " << bufferSizeValue << "\n"
                    << "Buffer size must be >= 7 characters\n";
          return false;
        }
        if (bufferSizeValue > 100) {
          std::cerr << "Warning: Very large buffer size (" << bufferSizeValue
                    << ") may cause precedence issues. Consider using a smaller size.\n";
        }
        bufferSize = bufferSizeValue;
      } catch (const std::exception& exception) {
        std::cerr << "Error: Invalid buffer size format: " << value << "\n";
        return false;
      }
    } else {
      std::cerr << "Error: Unknown argument: " << flag << "\n";
      return false;
    }
  }

  return true;
}

bool WinDozer::initAppData() {
  // Get the Windows AppData directory from environment
  // variable Example: C:\Users\Username\AppData\Roaming
  const char* appDataEnv = getenv("APPDATA");
  if (!appDataEnv) {
    std::cerr << "ERROR: APPDATA environment variable not found.\n";
    return false;
  }

  // Build the winDozer application directory path
  // Example: C:\Users\Username\AppData\Roaming\winDozer
  appDataPath = appDataEnv;
  appDataPath += "\\winDozer";

  // Create the winDozer directory if it doesn't exist
  // Note: Safe to call even if directory already exists
  try {
    if (!std::filesystem::exists(appDataPath)) {
      std::filesystem::create_directory(appDataPath);
    }
  } catch (const std::filesystem::filesystem_error& exception) {
    std::cerr << "ERROR: Failed to create application directory: " << appDataPath << "\n";
    std::cerr << "Filesystem error: " << exception.what() << "\n";
    return false;
  }

  // Build the settings file path
  // Example: C:\Users\Username\AppData\Roaming\winDozer\settings.ini
  settingsPath = appDataPath;
  settingsPath += "\\settings.ini";

  return true;
}

bool WinDozer::excludeOthers() {
  std::string lockPath = appDataPath;
  lockPath.append("\\lock");

  hLockFile = CreateFileA(lockPath.c_str(), (GENERIC_READ | GENERIC_WRITE), 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);

  if (hLockFile == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    std::cerr << "ERROR: Failed to create lock file: " << lockPath << "\n";
    std::cerr << "Error code: " << error << "\n";
    if (error == ERROR_SHARING_VIOLATION || error == ERROR_ALREADY_EXISTS) {
      std::cerr << "Another instance of winDozer is "
                   "already running.\n";
    }
    return false;
  }

  // Keep the handle open for the lifetime of the program
  return true;
}

void WinDozer::cleanupLockFile() {
  if (hLockFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hLockFile);
    hLockFile = INVALID_HANDLE_VALUE;
    std::string lockPath = appDataPath;
    lockPath.append("\\lock");
    DeleteFileA(lockPath.c_str());
  }
}
