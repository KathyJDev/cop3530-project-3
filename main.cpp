#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <set>
#include "document.h"
#include "tokenizer.h"
#include "inverted_index.h"
#include "suffix_array.h"
#include "performance.h"
#include "utils.h"

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

void printSnippets(const std::string& content, const std::string& query, int context = 40) {
    std::string lower_content = content;
    std::string lower_query = query;
    std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    size_t pos = 0;
    bool found = false;
    while ((pos = lower_content.find(lower_query, pos)) != std::string::npos) {
        found = true;
        int start = static_cast<int>(pos) - context;
        int end = static_cast<int>(pos) + static_cast<int>(query.size()) + context;
        if (start < 0) start = 0;
        if (end > (int)content.size()) end = (int)content.size();
        std::cout << "..." << content.substr(start, end - start) << "...\n";
        pos += query.size();
    }
    if (!found) {
        std::cout << "No snippets found.\n";
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= 0x0004;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

    // --- GUI/Script Integration Mode ---
    if (argc >= 2) {
        std::string cmd = argv[1];
        std::string folder = ".";
        std::string ds = "inverted"; // default

        // Helper to parse folder and ds
        auto parse_folder_ds = [&](int base) {
            if (argc > base) folder = argv[base];
            if (argc > base + 2 && std::string(argv[base + 1]) == "--ds") ds = argv[base + 2];
        };

        if (cmd == "--index" && argc >= 3) {
            folder = argv[2];
            std::vector<Document> docs = loadDocuments(folder);
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);
            std::cout << "Indexed " << docs.size() << " documents.\n";
            return 0;
        }
        else if (cmd == "--search" && argc >= 3) {
            std::string query = argv[2];
            parse_folder_ds(3);
            std::vector<Document> docs = loadDocuments(folder);
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);
            std::vector<std::string> tokens = tokenize(query);

            std::vector<int> results;
            if (ds == "suffix") {
                results = (tokens.size() > 1) ? saIndex.searchPhrase(query) : saIndex.searchKeyword(query);
            } else {
                results = (tokens.size() > 1) ? invIndex.searchPhrase(query) : invIndex.searchKeyword(query);
            }

            // Deduplicate doc IDs while preserving order
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
            return 0;
        }
        else if (cmd == "--lucky" && argc >= 3) {
            std::string query = argv[2];
            parse_folder_ds(3);
            std::vector<Document> docs = loadDocuments(folder);
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);
            std::vector<std::string> tokens = tokenize(query);

            std::vector<int> results;
            if (ds == "suffix") {
                results = (tokens.size() > 1) ? saIndex.searchPhrase(query) : saIndex.searchKeyword(query);
            } else {
                results = (tokens.size() > 1) ? invIndex.searchPhrase(query) : invIndex.searchKeyword(query);
            }

            // Deduplicate doc IDs while preserving order
            std::vector<int> unique_docIds;
            std::set<int> seen;
            for (int docId : results) {
                if (seen.insert(docId).second) {
                    unique_docIds.push_back(docId);
                }
            }

            if (!unique_docIds.empty()) {
                int docId = unique_docIds[0];
                auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
                if (it != docs.end()) {
                    std::cout << it->content << "\n";
                }
            }
            return 0;
        }
        else if (cmd == "--add-file" && argc >= 3) {
            std::string filePath = argv[2];
            if (argc >= 4) folder = argv[3];
            std::ifstream in(filePath);
            if (!in) {
                std::cerr << "Could not open file: " << filePath << "\n";
                return 1;
            }
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            std::vector<Document> docs = loadDocuments(folder);
            int newId = docs.empty() ? 1 : docs.back().id + 1;
            docs.push_back(Document(newId, content));
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);
            std::cout << "Added and indexed file: " << filePath << "\n";
            return 0;
        }
        else if (cmd == "--snippets" && argc >= 4) {
            std::string query = argv[2];
            int docId = std::stoi(argv[3]);
            parse_folder_ds(4);
            std::vector<Document> docs = loadDocuments(folder);
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);

            auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
            if (it != docs.end()) {
                // You can use a more advanced snippet extraction for suffix if you wish
                printSnippets(it->content, query);
            } else {
                std::cout << "Document not found.\n";
            }
            return 0;
        }
        // If unknown arg, fall through to CLI
    }

    // --- CLI Mode (unchanged) ---
    std::vector<Document> docs;
    InvertedIndex invIndex;
    SuffixArray saIndex;
    bool indexed = false;

    while (true) {
        printMenu();
        int choice;
        std::cin >> choice;

        if (choice == 1) {
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
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; }
            std::string keyword;
            while (true) {
                std::cout << "Enter keyword (single word only): ";
                std::cin.ignore();
                std::getline(std::cin, keyword);
                std::vector<std::string> tokens = tokenize(keyword);
                if (tokens.size() != 1) {
                    std::cout << "Error: Please enter exactly one word for keyword search.\n";
                    continue;
                }
                keyword = tokens[0];
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
            if (!indexed) { std::cout << "Please index documents first!\n"; continue; }
            std::string phrase;
            while (true) {
                std::cout << "Enter phrase (two or more words): ";
                std::cin.ignore();
                std::getline(std::cin, phrase);
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
