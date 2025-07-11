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
#include "suffix_array.h"
#include "performance.h"
#include "utils.h"

// Prompts user to choose a search method (Inverted Index or Suffix Array)
int searchMethod() {
    while (true) {
        std::cout << "\nChoose search method:\n"
                  << " [1] Inverted Index\n"
                  << " [2] Suffix Array\n"
                  << "Enter your choice: ";
        int choice;
        std::cin >> choice;
        if (choice == 1 || choice == 2) return choice;
        std::cout << "Invalid choice!\n";
    }
}

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
        for (size_t i = 0; i < docIds.size(); ++i) {
            auto it = std::find_if(docs.begin(), docs.end(), [id=docIds[i]](const Document& d){ return d.id == id; });
            std::string preview = (it != docs.end() && it->content.size() > 40) ? it->content.substr(0, 40) + "..." : (it != docs.end() ? it->content : "");
            std::cout << "  [" << (i+1) << "] Document " << docIds[i] << ": " << preview << "\n";
        }
        std::cout << "  [0] Back to menu\n";
        std::cout << "Select a document number: ";
        int sel = -1;
        std::cin >> sel;
        if (sel == 0) return -1;
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
    SuffixArray saIndex;               // Suffix array to store suffixes of documents.
    bool indexed = false;              // Tracks whether documents have been indexed.

    while (true) {
        printMenu();
        int choice;
        std::cin >> choice;

        if (choice == 1) {
            // --- Document Indexing ---
            std::cout << "Enter path to documents: ";
            std::string path;
            std::cin >> path;
            docs = loadDocuments(path);

            Performance::startTimer();
            invIndex.buildIndex(docs);
            double invIndexTime = Performance::stopTimer();

            Performance::startTimer();
            saIndex.buildIndex(docs);
            double saIndexTime = Performance::stopTimer();

            Performance::log("Inverted Index built in", invIndexTime);
            Performance::log("Suffix Array built in", saIndexTime);
            indexed = true;
        }
        else if (choice == 2) {
            // --- Keyword Search ---
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; }
            std::string keyword;
            while (true) {
                std::cout << "Enter keyword (single word only): ";
                std::cin.ignore();
                std::getline(std::cin, keyword);

                // Tokenize and check if it's a single word
                std::vector<std::string> tokens = tokenize(keyword);
                if (tokens.size() != 1) {
                    std::cout << "Error: Please enter exactly one word for keyword search.\n";
                    continue;
                }
                keyword = tokens[0]; // Use the cleaned word
                break;
            }

            Performance::startTimer();
            std::vector<int> invResults = invIndex.searchKeyword(keyword);
            double invTime = Performance::stopTimer();

            Performance::startTimer();
            std::vector<int> saResults = saIndex.searchKeyword(keyword);
            double saTime = Performance::stopTimer();

            int method = searchMethod();
            if (method == 1) {
                std::cout << "\nInverted Index Results (" << invResults.size() << " docs, " << invTime << " ms):\n";
                if (invResults.empty()) {
                    std::cout << "No documents found containing your search.\n";
                    continue;
                }
            }
            else if (method == 2) {
                std::cout << "\nSuffix Array Results (" << saResults.size() << " docs, " << saTime << " ms):\n";
                if (saResults.empty()) {
                    std::cout << "No documents found containing your search.\n";
                    continue;
                }
            }

            while (true) {
                int selectedId = method == 1 ? selectDocument(invResults, docs) : selectDocument(saResults, docs);
                if (selectedId == -1) break;
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
            std::string phrase;
            while (true) {
                std::cout << "Enter phrase (two or more words): ";
                std::cin.ignore();
                std::getline(std::cin, phrase);

                // Tokenize and check if it's more than one word
                std::vector<std::string> tokens = tokenize(phrase);
                if (tokens.size() < 2) {
                    std::cout << "Error: Please enter two or more words for phrase search.\n";
                    continue;
                }
                break;
            }

            Performance::startTimer();
            std::vector<int> invResults = invIndex.searchPhrase(phrase);
            double invTime = Performance::stopTimer();

            Performance::startTimer();
            std::vector<int> saResults = saIndex.searchPhrase(phrase);
            double saTime = Performance::stopTimer();

            int method = searchMethod();
            if (method == 1) {
                std::cout << "\nInverted Index Results (" << invResults.size() << " docs, " << invTime << " ms):\n";
                if (invResults.empty()) {
                    std::cout << "No documents found containing your search.\n";
                    continue;
                }
            }
            else if (method == 2) {
                std::cout << "\nSuffix Array Results (" << saResults.size() << " docs, " << saTime << " ms):\n";
                if (saResults.empty()) {
                    std::cout << "No documents found containing your search.\n";
                    continue;
                }
            }

            while (true) {
                int selectedId = method == 1 ? selectDocument(invResults, docs) : selectDocument(saResults, docs);
                if (selectedId == -1) break;
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
            std::vector<std::string> testKeywords = {"the", "and", "science", "history"};
            std::vector<std::string> testPhrases = {"the quick", "end of", "quantum physics"};

            std::cout << "\n=== Keyword Search Benchmark ===\n";
            for (const auto& kw : testKeywords) {
                Performance::startTimer();
                auto invResults = invIndex.searchKeyword(kw);
                double invTime = Performance::stopTimer();

                Performance::startTimer();
                auto saResults = saIndex.searchKeyword(kw);
                double saTime = Performance::stopTimer();

                std::cout << "Query: '" << kw << "'\n";
                std::cout << "  Inverted Index: " << invResults.size() << " docs, " << invTime << " ms\n";
                std::cout << "  Suffix Array:   " << saResults.size() << " docs, " << saTime << " ms\n\n";
            }
            std::cout << "\n=== Phrase Search Benchmark ===\n";
            for (const auto& ph : testPhrases) {
                Performance::startTimer();
                auto invResults = invIndex.searchPhrase(ph);
                double invTime = Performance::stopTimer();

                Performance::startTimer();
                auto saResults = saIndex.searchPhrase(ph);
                double saTime = Performance::stopTimer();

                std::cout << "Query: \"" << ph << "\"\n";
                std::cout << "  Inverted Index: " << invResults.size() << " docs, " << invTime << " ms\n";
                std::cout << "  Suffix Array:   " << saResults.size() << " docs, " << saTime << " ms\n\n";
            }
        }
        else if (choice == 5) {
            invIndex.clear();
            saIndex.clear();
            break;
        }
        else {
            std::cout << "Invalid choice!\n";
        }
    }
    return 0;
}
