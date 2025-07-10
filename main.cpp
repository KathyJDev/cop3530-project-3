#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "document.h"
#include "tokenizer.h"
#include "inverted_index.h"
// #include "suffix_array.h" // TODO: Uncomment if/when suffix array support is implemented
#include "performance.h"
#include "utils.h"

// Prints the main menu to the console, showing available actions to the user.
void printMenu() {
    std::cout << "------------------------------------------\n"
              << "|          Simple Search Engine          |\n"
              << "------------------------------------------\n"
              << "| 1. Index Documents                     |\n"
              << "| 2. Search Keywords                     |\n"
              << "| 3. Search Phrase                       |\n"
              << "| 4. Performance Report                  |\n"
              << "| 5. Exit                                |\n"
              << "------------------------------------------\n"
              << "Enter your choice: ";
}

// Displays a numbered list of matching documents and lets the user select one to view.
// Returns the selected document's ID, or -1 if the user chooses to go back to the menu.
int selectDocument(const std::vector<int>& docIds, const std::vector<Document>& docs) {
    while (true) {
        std::cout << "\nDocuments containing your search:\n";
        // List each matching document with a brief content preview.
        for (size_t i = 0; i < docIds.size(); ++i) {
            auto it = std::find_if(docs.begin(), docs.end(), [id=docIds[i]](const Document& d){ return d.id == id; });
            std::string preview = (it != docs.end() && it->content.size() > 40) ? it->content.substr(0, 40) + "..." : (it != docs.end() ? it->content : "");
            std::cout << "  [" << (i+1) << "] Document " << docIds[i] << ": " << preview << "\n";
        }
        std::cout << "  [0] Back to menu\n";
        std::cout << "Select a document number: ";
        int sel = -1;
        std::cin >> sel;
        // If user selects 0, return -1 to indicate going back to main menu.
        if (sel == 0) return -1;
        // If valid document number selected, return the corresponding doc ID.
        if (sel > 0 && sel <= (int)docIds.size()) return docIds[sel-1];
        std::cout << "Invalid selection. Try again.\n";
    }
}

int main() {
#ifdef _WIN32
    // Enable ANSI color in Windows 10+ cmd.exe for colored output.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

    std::vector<Document> docs;        // Stores all loaded documents.
    InvertedIndex invIndex;            // Inverted index for fast keyword/phrase searching.
    // SuffixArray saIndex;            // TODO: Uncomment if/when suffix array support is implemented.
    bool indexed = false;              // Tracks whether documents have been indexed.

    // Main program loop: displays menu and handles user choices.
    while (true) {
        printMenu();                   // Show the main menu.
        int choice;
        std::cin >> choice;            // Get user menu choice.

        if (choice == 1) {
            // --- Document Indexing ---
            std::cout << "Enter path to documents: ";
            std::string path;
            std::cin >> path;
            // Load all .txt files from the specified directory.
            docs = loadDocuments(path);

            // Build the inverted index and measure performance.
            Performance::startTimer();
            invIndex.buildIndex(docs);
            double invIndexTime = Performance::stopTimer();

            // TODO: Uncomment to build suffix array index and measure performance.
            // Performance::startTimer();
            // saIndex.buildIndex(docs);
            // double saIndexTime = Performance::stopTimer();

            // Display timing information to the user.
            Performance::log("Inverted Index built in", invIndexTime);
            // TODO: Uncomment to log suffix array build time.
            // Performance::log("Suffix Array built in", saIndexTime);
            indexed = true; // Mark as indexed.
        }
        else if (choice == 2) {
            // --- Keyword Search ---
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; }
            std::cout << "Enter keyword: ";
            std::string keyword;
            std::cin.ignore(); // Clear newline left in input buffer.
            std::getline(std::cin, keyword); // Get the search keyword from the user.

            // Search for the keyword using the inverted index and time the operation.
            Performance::startTimer();
            std::vector<int> invResults = invIndex.searchKeyword(keyword);
            double invTime = Performance::stopTimer();

            // TODO: Uncomment to search using suffix array and measure time.
            // Performance::startTimer();
            // std::vector<int> saResults = saIndex.searchKeyword(keyword);
            // double saTime = Performance::stopTimer();

            std::cout << "\nInverted Index Results (" << invResults.size() << " docs, " << invTime << " ms):\n";
            if (invResults.empty()) continue; // If no results, return to menu.

            // Allow repeated document selection and snippet viewing until user chooses to return.
            while (true) {
                int selectedId = selectDocument(invResults, docs);
                if (selectedId == -1) break; // User chose to go back to menu.
                // Find the selected document and show all matching snippets.
                auto it = std::find_if(docs.begin(), docs.end(), [selectedId](const Document& d){ return d.id == selectedId; });
                if (it != docs.end()) {
                    std::cout << "Document " << selectedId << ":\n";
                    showAllSnippets(it->content, keyword);
                }
            }
        }
        else if (choice == 3) {
            // --- Phrase Search ---
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; }
            std::cout << "Enter phrase: ";
            std::string phrase;
            std::cin.ignore(); // Clear newline left in input buffer.
            std::getline(std::cin, phrase); // Get the search phrase from the user.

            // Search for the phrase using the inverted index and time the operation.
            Performance::startTimer();
            std::vector<int> invResults = invIndex.searchPhrase(phrase);
            double invTime = Performance::stopTimer();

            // TODO: Uncomment to search using suffix array and measure time.
            // Performance::startTimer();
            // std::vector<int> saResults = saIndex.searchPhrase(phrase);
            // double saTime = Performance::stopTimer();

            std::cout << "\nInverted Index Results (" << invResults.size() << " docs, " << invTime << " ms):\n";
            if (invResults.empty()) continue; // If no results, return to menu.

            // Allow repeated document selection and snippet viewing until user chooses to return.
            while (true) {
                int selectedId = selectDocument(invResults, docs);
                if (selectedId == -1) break; // User chose to go back to menu.
                // Find the selected document and show all matching snippets.
                auto it = std::find_if(docs.begin(), docs.end(), [selectedId](const Document& d){ return d.id == selectedId; });
                if (it != docs.end()) {
                    std::cout << "Document " << selectedId << ":\n";
                    showAllSnippets(it->content, phrase);
                }
            }
        }
        else if (choice == 4) {
            // --- Performance Report ---
            if (!indexed) { std::cout << "Index documents first!\n"; continue; }
            // Predefined test cases for benchmarking search performance.
            std::vector<std::string> testKeywords = {"the", "and", "science", "history"};
            std::vector<std::string> testPhrases = {"the quick", "end of", "quantum physics"};

            std::cout << "\n=== Keyword Search Benchmark ===\n";
            for (const auto& kw : testKeywords) {
                Performance::startTimer();
                auto invResults = invIndex.searchKeyword(kw);
                double invTime = Performance::stopTimer();

                // TODO: Uncomment to benchmark suffix array.
                // Performance::startTimer();
                // auto saResults = saIndex.searchKeyword(kw);
                // double saTime = Performance::stopTimer();

                std::cout << "Query: '" << kw << "'\n";
                std::cout << "  Inverted Index: " << invResults.size() << " docs, " << invTime << " ms\n";
                // TODO: Uncomment to show suffix array results.
                // std::cout << "  Suffix Array:   " << saResults.size() << " docs, " << saTime << " ms\n\n";
            }
            std::cout << "\n=== Phrase Search Benchmark ===\n";
            for (const auto& ph : testPhrases) {
                Performance::startTimer();
                auto invResults = invIndex.searchPhrase(ph);
                double invTime = Performance::stopTimer();

                // TODO: Uncomment to benchmark suffix array.
                // Performance::startTimer();
                // auto saResults = saIndex.searchPhrase(ph);
                // double saTime = Performance::stopTimer();

                std::cout << "Query: \"" << ph << "\"\n";
                std::cout << "  Inverted Index: " << invResults.size() << " docs, " << invTime << " ms\n";
                // TODO: Uncomment to show suffix array results.
                // std::cout << "  Suffix Array:   " << saResults.size() << " docs, " << saTime << " ms\n\n";
            }
        }
        else if (choice == 5) {
            // --- Exit Program ---
            invIndex.clear(); // Free memory used by the inverted index.
            // TODO: Uncomment to clear suffix array index.
            // saIndex.clear();
            break; // Exit the main loop and terminate the program.
        }
        else {
            // Handle invalid menu selections.
            std::cout << "Invalid choice!\n";
        }
    }
    return 0;
}
