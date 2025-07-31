#include "utils.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    #include <tchar.h>
    #include <shlwapi.h>
    #include <conio.h>
    #pragma comment(lib, "shlwapi.lib")
    #undef max
    #undef min

    // Helper: Converts a std::string to std::wstring for Windows API compatibility.
    std::wstring stringToWString(const std::string& s) {
        int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, 0, 0);
        std::wstring ws(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &ws[0], len);
        if (!ws.empty() && ws.back() == L'\0') ws.pop_back(); // Remove null terminator if present
        return ws;
    }

    // Helper: Converts a std::wstring to std::string for file path manipulation.
    std::string wstringToString(const std::wstring& ws) {
        int len = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, 0, 0, 0, 0);
        std::string s(len, '\0');
        WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, &s[0], len, 0, 0);
        if (!s.empty() && s.back() == '\0') s.pop_back(); // Remove null terminator if present
        return s;
    }
#else
    #include <dirent.h>
    #include <sys/types.h>
    #include <cstring>
    #include <termios.h>
    #include <unistd.h>
    // Reads a single character from stdin without waiting for Enter (UNIX).
    char getch() {
        char buf = 0;
        struct termios old = {};
        if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
        struct termios newt = old;
        newt.c_lflag &= ~ICANON; // Disable canonical mode (buffered I/O)
        newt.c_lflag &= ~ECHO;   // Disable echo
        if (tcsetattr(0, TCSANOW, &newt) < 0) perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0) perror ("read()");
        tcsetattr(0, TCSANOW, &old); // Restore terminal settings
        return buf;
    }
#endif

// Loads all .txt files from the specified directory and returns a vector of Document objects.
// Each document is assigned a unique integer ID starting from 1.
std::vector<Document> loadDocuments(const std::string& dirPath) {
    std::vector<Document> docs;
    int id = 1;
#ifdef _WIN32
    // Build a search pattern for .txt files in the given directory.
    std::string pattern = dirPath + "\\*.txt";
    std::wstring wpattern = stringToWString(pattern);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(wpattern.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "Directory does not exist or could not be opened: " << dirPath << std::endl;
        return docs;
    }
    do {
        // Skip directories, only process files.
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring wfname = ffd.cFileName;
            std::string fname = dirPath + "\\" + wstringToString(wfname);
            std::ifstream file(fname.c_str());
            if (!file.is_open()) continue; // Skip unreadable files
            // Read the entire file content into a string.
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            docs.emplace_back(id++, content); // Add document to vector
        }
    } while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);
#else
    DIR* dir = opendir(dirPath.c_str());
    // Directory could not be opened
    if (!dir) {
        std::cout << "Directory does not exist or could not be opened: " << dirPath << std::endl;
        return docs;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fname(entry->d_name);
        // Only process files ending with ".txt"
        if (fname.length() > 4 && fname.substr(fname.length()-4) == ".txt") {
            std::ifstream file((dirPath + "/" + fname).c_str());
            if (!file.is_open()) continue; // Skip unreadable files
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            docs.emplace_back(id++, content); // Add document to vector
        }
    }
    closedir(dir);
#endif
    return docs;
}

// Generates a snippet from the content, centered around the first occurrence of the query.
// The snippet will be at most snippetLength characters long.
// If the query is not found, returns an empty string.
std::string generateSnippet(const std::string& content, const std::string& query, int snippetLength) {
    size_t pos = content.find(query);
    if (pos == std::string::npos) return ""; // Query not found
    // Calculate the start and end positions for the snippet window.
    int start = std::max(0, static_cast<int>(pos) - snippetLength/2);
    int end = std::min(static_cast<int>(content.size()), start + snippetLength);
    return content.substr(start, end - start) + "...";
}

// Highlights all occurrences of the keyword (case-insensitive) in the text using ANSI color codes.
// Returns the highlighted string with the keyword in yellow.
std::string highlightKeyword(const std::string& text, const std::string& keyword) {
    std::string result;
    size_t pos = 0, last = 0;
    // Convert both text and keyword to lowercase for case-insensitive matching.
    std::string lowerText = text, lowerKeyword = keyword;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    // Iterate through all matches of the keyword in the text.
    while ((pos = lowerText.find(lowerKeyword, last)) != std::string::npos) {
        result += text.substr(last, pos - last); // Add text before match
        // Add the matched keyword highlighted in yellow (ANSI escape code)
        result += "\033[1;33m" + text.substr(pos, keyword.length()) + "\033[0m";
        last = pos + keyword.length(); // Move past this match
    }
    result += text.substr(last); // Add any remaining text after the last match
    return result;
}

// Shows all snippets for each occurrence of the keyword/phrase in the content.
// Allows user to scroll between occurrences using 'a' (prev), 'd' (next), and 'q' (quit).
// The snippet is centered on the occurrence and highlighted.
void showAllSnippets(const std::string& content, const std::string& keyword, int snippetLength) {
    // Prepare lowercase versions for case-insensitive search.
    std::string lowerContent = content, lowerKeyword = keyword;
    std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    // Find all starting positions of the keyword in the content.
    std::vector<size_t> positions;
    size_t pos = 0;
    while ((pos = lowerContent.find(lowerKeyword, pos)) != std::string::npos) {
        positions.push_back(pos);
        pos += keyword.length(); // Move past this match for next search
    }

    // If no matches, inform the user and return.
    if (positions.empty()) {
        std::cout << "  No occurrences found in this document.\n";
        return;
    }

    int idx = 0; // Index of the current occurrence being shown
    while (true) {
        // Clear the terminal screen before showing each snippet for clarity.
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        // Calculate snippet boundaries, centered on the current occurrence.
        int start = std::max(0, static_cast<int>(positions[idx]) - snippetLength / 2);
        int end = std::min(static_cast<int>(content.size()), start + snippetLength);
        std::string snippet = content.substr(start, end - start);

        // Display the occurrence number, total occurrences, and the highlighted snippet.
        std::cout << "Occurrence " << (idx + 1) << " of " << positions.size() << ":\n";
        std::cout << highlightKeyword(snippet, keyword) << "\n";
        std::cout << "[a: prev | d: next | q: quit]\n";

        // Wait for user input to navigate or quit (no Enter required).
        char ch = 0;
#ifdef _WIN32
        ch = _getch(); // Windows: read keypress
#else
        ch = getch();  // Linux/macOS: read keypress
#endif

        // Handle navigation: 'a' for previous, 'd' for next, 'q' to quit.
        if (ch == 'a' || ch == 'A') {
            if (idx > 0) idx--; // Go to previous occurrence if possible
        } else if (ch == 'd' || ch == 'D') {
            if (idx < (int)positions.size() - 1) idx++; // Go to next occurrence if possible
        } else if (ch == 'q' || ch == 'Q') {
            break; // Exit snippet viewer
        }
        // Any other key is ignored
    }
}
