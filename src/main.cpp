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
#include <cstdio>    // For std::rename
#include <regex>     // For sanitizing filenames

#include "document.h"
#include "tokenizer.h"
#include "inverted_index.h"
#include "suffix_array.h"
#include "performance.h"
#include "utils.h" // Include utils.h for showAllSnippets

// Function to prompt user to choose between Inverted Index and Suffix Array
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

// Helper function to clean a string to be a valid filename
std::string sanitizeFilename(std::string name) {
    // Trim leading/trailing whitespace
    name.erase(0, name.find_first_not_of(" \t\n\r"));
    name.erase(name.find_last_not_of(" \t\n\r") + 1);

    // Replace invalid filename characters (e.g., :, /, \, ?, *) with an underscore
    std::regex re(R"([<>:"/\\|?*])");
    name = std::regex_replace(name, re, "_");
    
    // Limit filename length to avoid OS issues
    if (name.length() > 100) {
        name = name.substr(0, 100);
    }
    
    return name;
}

// Helper function to extract the title from a Gutenberg text file
std::string extractTitle(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ""; // Return empty if file can't be opened
    }

    std::string line;
    std::string title;
    const std::string title_prefix = "Title: ";
    int lines_to_check = 50; // Only search the top of the file for the title

    while (lines_to_check > 0 && std::getline(file, line)) {
        // Check if the line starts with "Title: "
        if (line.rfind(title_prefix, 0) == 0) { 
            title = line.substr(title_prefix.length());
            break; // Found it, no need to look further
        }
        lines_to_check--;
    }

    file.close();

    if (!title.empty()) {
        return sanitizeFilename(title);
    }
    
    return ""; // Return empty if no title was found
}

// Function to download a book from Project Gutenberg and save it by its title
void downloadGutenbergBook() {
    std::cout << "\nEnter the path to a folder that will *contain* 'test_data/' (e.g., 'my_books').\n"
                 "The 'test_data/' folder will be created inside this path to hold the Gutenberg books.\n"
                 "Press Enter to use the current directory '.' as the container folder (resulting in 'test_data/', the provided folder): ";
    std::string folderPath;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    // Use std::getline to read the entire line, allowing for empty input
    std::getline(std::cin, folderPath);

    // If the input path is empty, default it to the current directory ('.')
    if (folderPath.empty()) {
        folderPath = ".";
    }

    // storePath will be "data/test_data" if folderPath is "data"
    std::string storePath = folderPath + "/test_data";
#ifdef _WIN32
    std::string mkdir_cmd = "if not exist \"" + storePath + "\" mkdir \"" + storePath + "\" > nul 2>&1";
#else
    std::string mkdir_cmd = "mkdir -p \"" + storePath + "\"";
#endif
    // It's good practice to check the result of system()
    int mkdir_result = system(mkdir_cmd.c_str());
    if (mkdir_result != 0) {
        std::cerr << "Error: Failed to create or access directory '" << storePath << "'. Please check permissions or path.\n";
        return; // Exit if directory cannot be created/accessed
    }

    std::cout << "Enter the Project Gutenberg Book ID (e.g., 84 for Frankenstein): ";
    int bookId;
    std::cin >> bookId;

    if (std::cin.fail() || bookId <= 0) {
        std::cout << "Invalid Book ID. Please enter a positive number.\n";
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        return;
    }
    std::cin.ignore(); // Consume the newline character left by previous std::cin or other inputs

    // Download to a temporary file first
    std::string temp_file_path = storePath + "/temp_book_download.txt"; // e.g., "data/test_data/temp_book_download.txt"
    std::string url = "https://www.gutenberg.org/cache/epub/" + std::to_string(bookId) + "/pg" + std::to_string(bookId) + ".txt";
    std::string command = "curl -L -o \"" + temp_file_path + "\" \"" + url + "\"";

    std::cout << "\nDownloading book " << bookId << " from " << url << "...\n";
    int result = system(command.c_str());

    if (result != 0) {
        std::cout << "\nFailed to download the book. Make sure 'curl' is installed and in your PATH, and the Book ID is correct.\n";
        remove(temp_file_path.c_str()); // Clean up temp file on failure
        return;
    }

    // Extract title to determine the final filename
    std::string title = extractTitle(temp_file_path);
    std::string final_filename;

    if (!title.empty()) {
        final_filename = title + ".txt";
    } else {
        std::cout << "Could not automatically find a title. Using default name.\n";
        final_filename = "pg" + std::to_string(bookId) + ".txt";
    }

    std::string final_filepath = storePath + "/" + final_filename; // e.g., "data/test_data/Frankenstein.txt"

    // 1. Read content from the TEMPORARY file (this is where the downloaded data is)
    std::ifstream temp_file_in(temp_file_path);
    if (!temp_file_in.is_open()) {
        std::cerr << "Error: Could not open temporary downloaded file for reading: " << temp_file_path << "\n";
        // Do not remove temp_file_path here, it implies it couldn't even be opened for reading.
        return;
    }
    std::string content((std::istreambuf_iterator<char>(temp_file_in)), std::istreambuf_iterator<char>());
    temp_file_in.close(); // Close the temp file after reading its content

    // 2. Prepend the filename (title) to the content
    content = final_filename + "\n\n" + content;

    // 3. Write the modified content to the FINAL destination file
    std::ofstream final_file_out(final_filepath);
    if (!final_file_out.is_open()) {
        std::cerr << "Error: Could not open final destination file for writing: " << final_filepath << "\n";
        remove(temp_file_path.c_str()); // Clean up temp file since we can't write final
        return;
    }
    final_file_out << content;
    final_file_out.close(); // Close the final file after writing

    // 4. Clean up the temporary file (no need for std::rename anymore, as content was already written to final_filepath)
    remove(temp_file_path.c_str());

    std::cout << "Successfully downloaded and saved as '" << final_filename << "' in '" << storePath << "'.\n"
              << "Please re-index your documents (Option 1) to include this new book.\n";
}

// Prints the main menu options to the console.
void printMenu() {
    std::cout << "------------------------------------------\n"
              << "|         Simple Search Engine           |\n"
              << "------------------------------------------\n"
              << "| 1. Index Documents                     |\n"
              << "| 2. Search Keywords                     |\n"
              << "| 3. Search Phrase                       |\n"
              << "| 4. Performance Report                  |\n"
              << "| 5. Download Book from Gutenberg        |\n"
              << "| 6. Exit                                |\n"
              << "------------------------------------------\n"
              << "Enter your choice: ";
}

// Allows the user to select a document from a list of search results.
// Displays document previews and returns the selected document ID or -1 to go back.
int selectDocument(const std::vector<int>& docIds, const std::vector<Document>& docs) {
    while (true) {
        std::cout << "\nDocuments containing your search:\n";
        for (size_t i = 0; i < docIds.size(); ++i) {
            // Find the document object corresponding to the ID
            auto it = std::find_if(docs.begin(), docs.end(), [id=docIds[i]](const Document& d){ return d.id == id; });
            // Create a short preview of the document content
            std::string preview = (it != docs.end() && it->content.size() > 40) ? it->content.substr(0, 40) + "..." : (it != docs.end() ? it->content : "");
            std::cout << "  [" << (i+1) << "] Document " << docIds[i] << ": " << preview << "\n";
        }
        std::cout << "  [0] Back to menu\n";
        std::cout << "Select a document number: ";
        int sel = -1;
        std::cin >> sel;
        if (sel == 0) return -1; // User chose to go back
        if (sel > 0 && sel <= (int)docIds.size()) return docIds[sel-1]; // Return selected document ID
        std::cout << "Invalid selection. Try again.\n";
    }
}

// Prints all non-interactive snippets of the query in the content.
// Used for command-line argument mode where interactive scrolling is not desired.
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

        // Lambda helper to parse folder path and data structure argument
        auto parse_folder_ds = [&](int base) {
            if (argc > base) folder = argv[base];
            if (argc > base + 2 && std::string(argv[base + 1]) == "--ds") ds = argv[base + 2];
        };

        // Command: --index <folder_path>
        if (cmd == "--index" && argc >= 3) {
            folder = argv[2];
            std::vector<Document> docs = loadDocuments(folder);
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);
            std::cout << "Indexed " << docs.size() << " documents.\n";
            return 0;
        }
        // Command: --search <query> [--folder <folder_path>] [--ds <inverted|suffix>]
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
            } else { // Default to inverted
                results = (tokens.size() > 1) ? invIndex.searchPhrase(query) : invIndex.searchKeyword(query);
            }

            // Deduplicate document IDs while preserving order of first appearance
            std::vector<int> unique_docIds;
            std::set<int> seen;
            for (int docId : results) {
                if (seen.insert(docId).second) { // If insertion successful, it's a new ID
                    unique_docIds.push_back(docId);
                }
            }

            // Print document previews for search results
            for (int docId : unique_docIds) {
                auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
                if (it != docs.end()) {
                    std::string preview = (it->content.size() > 80) ? it->content.substr(0, 80) + "..." : it->content;
                    std::cout << "Document " << docId << ": " << preview << "\n";
                }
            }
            return 0;
        }
        // Command: --lucky <query> [--folder <folder_path>] [--ds <inverted|suffix>]
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
            } else { // Default to inverted
                results = (tokens.size() > 1) ? invIndex.searchPhrase(query) : invIndex.searchKeyword(query);
            }

            // Deduplicate document IDs while preserving order
            std::vector<int> unique_docIds;
            std::set<int> seen;
            for (int docId : results) {
                if (seen.insert(docId).second) {
                    unique_docIds.push_back(docId);
                }
            }

            // Print the full content of the first found document
            if (!unique_docIds.empty()) {
                int docId = unique_docIds[0];
                auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
                if (it != docs.end()) {
                    std::cout << it->content << "\n";
                }
            }
            return 0;
        }
        // Command: --add-file <file_path> [--folder <target_folder>]
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
            // Assign a new ID; if docs is empty, start from 1, otherwise increment last ID
            int newId = docs.empty() ? 1 : docs.back().id + 1;
            docs.push_back(Document(newId, content));
            InvertedIndex invIndex; invIndex.buildIndex(docs); // Rebuild index with new document
            SuffixArray saIndex; saIndex.buildIndex(docs);     // Rebuild index with new document
            std::cout << "Added and indexed file: " << filePath << "\n";
            return 0;
        }
        // Command: --snippets <query> <doc_id> [--folder <folder_path>] [--ds <inverted|suffix>]
        else if (cmd == "--snippets" && argc >= 4) {
            std::string query = argv[2];
            int docId = std::stoi(argv[3]); // Convert document ID string to integer
            parse_folder_ds(4);
            std::vector<Document> docs = loadDocuments(folder);
            InvertedIndex invIndex; invIndex.buildIndex(docs);
            SuffixArray saIndex; saIndex.buildIndex(docs);

            auto it = std::find_if(docs.begin(), docs.end(), [docId](const Document& d){ return d.id == docId; });
            if (it != docs.end()) {
                // Use the non-interactive printSnippets for command-line output
                printSnippets(it->content, query);
            } else {
                std::cout << "Document not found.\n";
            }
            /*
        }else if (cmd == "--get-content" && argc >= 4) {
            int docId = std::stoi(argv[2]);
            folder = argv[3];
            std::vector<Document> docs = loadDocuments(folder);

            auto it = std::find_if(docs.begin(),docs.end(), [docId](const Document& d){ return d.id == docId; });
            if (it != docs.end()) {
                cout<<it->content<<"\n";
            }else {
                return 1;
            }
        */
            return 0;
        }
        // If an unknown command-line argument is provided, fall through to CLI mode.
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
