#ifdef _WIN32
#define NOMINMAX // Prevents Windows.h from defining min/max macros
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <set>
#include <cstdlib>   // For system()
#include <limits>    // For std::numeric_limits

#include "document.h"
#include "tokenizer.h"
#include "inverted_index.h"
#include "suffix_array.h"
#include "performance.h"
#include "menu.h"
#include "gutenberg.h"
#include "utils.h"

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Enable ANSI escape codes for colored output on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

            // --- GUI/Script Integration Mode ---
    // Handles command-line arguments for non-interactive operations.
    if (argc >= 2) {
        std::string cmd = argv[1];
        std::string folder = "."; // Default folder is current directory
        std::string ds = "inverted"; // Default data structure for search

        auto parse_folder_ds = [&](int base) {
            if (argc > base) folder = argv[base];
            if (argc > base + 2 && std::string(argv[base + 1]) == "--ds") ds = argv[base + 2];
        };

        // --- --index ---
        // Builds the indexes and saves them to disk.
        if (cmd == "--index" && argc >= 3) {
            folder = argv[2];
            std::cout << "Indexing documents in " << folder << "...\n";
            std::vector<Document> docs = loadDocuments(folder);
            
            InvertedIndex invIndex; 
            invIndex.buildIndex(docs);
            invIndex.save("inverted_index.dat");

            SuffixArray saIndex; 
            saIndex.buildIndex(docs);
            saIndex.save("suffix_array.dat");

            std::cout << "Indexed " << docs.size() << " documents and saved indexes to disk.\n";
            return 0;
        }

        // --- --search & --snippets ---
        // These now load the pre-built index instead of rebuilding it.
        else if (cmd == "--search" || cmd == "--snippets") {
            if (argc < 3) {
                std::cerr << "Error: Not enough arguments for " << cmd << "\n";
                return 1;
            }
            std::string query = argv[2];

            // Load PRE-BUILT indexes from disk
            InvertedIndex invIndex;
            if (!invIndex.load("inverted_index.dat")) {
                std::cerr << "Error: Inverted Index file not found. Please run --index first.\n";
                return 1;
            }
            SuffixArray saIndex;
            if (!saIndex.load("suffix_array.dat")) {
                std::cerr << "Error: Suffix Array index file not found. Please run --index first.\n";
                return 1;
            }

            // Load documents only for displaying content, not for indexing
            if (cmd == "--search") {
                 parse_folder_ds(3);
                 std::vector<Document> docs = loadDocuments(folder);
                 std::vector<std::string> tokens = tokenize(query);

                 std::vector<int> results;
                 if (ds == "suffix") {
                     results = (tokens.size() > 1) ? saIndex.searchPhrase(query) : saIndex.searchKeyword(query);
                 } else {
                     results = (tokens.size() > 1) ? invIndex.searchPhrase(query) : invIndex.searchKeyword(query);
                 }
                
                std::vector<int> unique_docIds;
                std::set<int> seen;
                for (int docId : results) {
                    if (seen.insert(docId).second) {
                        unique_docIds.push_back(docId);
                    }
                }
                
                for (int docId : unique_docIds) {
                    auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
                    if (it != docs.end()) {
                        std::string preview = (it->content.size() > 80) ? it->content.substr(0, 80) + "..." : it->content;
                        std::cout << "Document " << docId << ": " << preview << "\n";
                    }
                }
            } else { // --snippets
                 if (argc < 4) {
                     std::cerr << "Error: --snippets requires a document ID.\n";
                     return 1;
                 }
                 int docId = std::stoi(argv[3]);
                 parse_folder_ds(4);
                 std::vector<Document> docs = loadDocuments(folder);
                 auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
                 if (it != docs.end()) {
                     printSnippets(it->content, query);
                 } else {
                     std::cout << "Document not found.\n";
                 }
            }
            return 0;
        }

        // --- --get-content ---
        // Retrieves the full content of a single document by its ID.
        else if (cmd == "--get-content" && argc >= 3) {
            int docId = std::stoi(argv[2]);
            if (argc >= 4) folder = argv[3];
            
            std::vector<Document> docs = loadDocuments(folder);
            auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });

            if (it != docs.end()) {
                std::cout << it->content << std::endl;
                return 0;
            } else {
                std::cerr << "Error: Document with ID " << docId << " not found." << std::endl;
                return 1;
            }
        }

        // --- --add-file ---
        // Adds a new file, rebuilds the index, and re-saves it to disk.
        else if (cmd == "--add-file" && argc >= 3) {
            std::string filePath = argv[2];
            if (argc >= 4) folder = argv[3];

            std::vector<Document> docs = loadDocuments(folder);
            
            std::ifstream in(filePath);
            if (!in) {
                std::cerr << "Could not open file: " << filePath << "\n";
                return 1;
            }
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            
            int newId = docs.empty() ? 1 : docs.back().id + 1;
            docs.push_back(Document(newId, content));
            
            InvertedIndex invIndex; 
            invIndex.buildIndex(docs);
            invIndex.save("inverted_index.dat");
            
            SuffixArray saIndex; 
            saIndex.buildIndex(docs);
            saIndex.save("suffix_array.dat");
            
            std::cout << "Added and re-indexed file: " << filePath << "\n";
            return 0;
        }
    }

    // --- CLI Mode ---
    // Interactive command-line interface for the search engine.
    std::vector<Document> docs;
    InvertedIndex invIndex;
    SuffixArray saIndex;
    bool indexed = false; // Flag to check if documents have been indexed

    while (true) {
        printMenu(); // Display the main menu
        int choice;
        std::cin >> choice;
        
        // Input validation for menu choice
        if (std::cin.fail()) {
            std::cout << "Invalid input. Please enter a number.\n";
            std::cin.clear(); // Clear error flags
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); // Discard invalid input
            continue; // Loop back to menu
        }

        if (choice == 1) { // Index Documents
            std::cout << "Enter path to documents: ";
            std::string path;
            std::cin >> path; // Read document folder path
            docs = loadDocuments(path); // Load documents from path

            // Benchmark Inverted Index building
            Performance::startTimer();
            invIndex.buildIndex(docs);
            double invIndexTime = Performance::stopTimer();

            // Benchmark Suffix Array building
            Performance::startTimer();
            saIndex.buildIndex(docs);
            double saIndexTime = Performance::stopTimer();

            Performance::log("Inverted Index built in", invIndexTime);
            Performance::log("Suffix Array built in", saIndexTime);
            indexed = true; // Set indexed flag to true
        }
        else if (choice == 2) { // Search Keywords
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; } // Check if documents are indexed
            std::string keyword;
            while (true) {
                std::cout << "Enter keyword (single word only): ";
                std::cin.ignore(); // Consume the newline character left by previous std::cin
                std::getline(std::cin, keyword); // Read the entire line for the keyword
                std::vector<std::string> tokens = tokenize(keyword);
                if (tokens.size() != 1) {
                    std::cout << "Error: Please enter exactly one word for keyword search.\n";
                    continue; // Re-prompt if not a single word
                }
                keyword = tokens[0]; // Use the tokenized keyword
                break;
            }
            // Benchmark Inverted Index keyword search
            Performance::startTimer();
            std::vector<int> invResults = invIndex.searchKeyword(keyword);
            double invTime = Performance::stopTimer();

            // Benchmark Suffix Array keyword search
            Performance::startTimer();
            std::vector<int> saResults = saIndex.searchKeyword(keyword);
            double saTime = Performance::stopTimer();

            int method = searchMethod(); // Ask user to choose search method
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

            // Loop for selecting and viewing document snippets
            while (true) {
                int selectedId = method == 1 ? selectDocument(invResults, docs) : selectDocument(saResults, docs);
                if (selectedId == -1) break; // User chose to go back
                auto it = std::find_if(docs.begin(), docs.end(), [selectedId](const Document& d){ return d.id == selectedId; });
                if (it != docs.end()) {
                    std::cout << "Document " << selectedId << ":\n";
                    // Use the interactive showAllSnippets from utils.h/cpp
                    showAllSnippets(it->content, keyword); 
                }
            }
        }
        else if (choice == 3) { // Search Phrase
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; } // Check if documents are indexed
            std::string phrase;
            while (true) {
                std::cout << "Enter phrase (two or more words): ";
                std::cin.ignore(); // Consume the newline character
                std::getline(std::cin, phrase); // Read the entire line for the phrase
                std::vector<std::string> tokens = tokenize(phrase);
                if (tokens.size() < 2) {
                    std::cout << "Error: Please enter two or more words for phrase search.\n";
                    continue; // Re-prompt if not enough words
                }
                break;
            }
            // Benchmark Inverted Index phrase search
            Performance::startTimer();
            std::vector<int> invResults = invIndex.searchPhrase(phrase);
            double invTime = Performance::stopTimer();

            // Benchmark Suffix Array phrase search
            Performance::startTimer();
            std::vector<int> saResults = saIndex.searchPhrase(phrase);
            double saTime = Performance::stopTimer();

            int method = searchMethod(); // Ask user to choose search method
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

            // Loop for selecting and viewing document snippets
            while (true) {
                int selectedId = method == 1 ? selectDocument(invResults, docs) : selectDocument(saResults, docs);
                if (selectedId == -1) break; // User chose to go back
                auto it = std::find_if(docs.begin(), docs.end(), [selectedId](const Document& d){ return d.id == selectedId; });
                if (it != docs.end()) {
                    std::cout << "Document " << selectedId << ":\n";
                    // Use the interactive showAllSnippets from utils.h/cpp
                    showAllSnippets(it->content, phrase); 
                }
            }
        }
        else if (choice == 4) { // Performance Report
            if (!indexed) { std::cout << "Index documents first!\n"; continue; } // Ensure documents are indexed
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
        else if (choice == 5) { // Download Book from Gutenberg
            downloadGutenbergBook(); // Call the new function to download books
        }
        else if (choice == 6) { // Exit
            invIndex.clear(); // Clear index data before exiting
            saIndex.clear();
            break; // Exit the main loop
        }
        else {
            std::cout << "Invalid choice!\n"; // Handle invalid menu input
        }
    }
    return 0;
}
