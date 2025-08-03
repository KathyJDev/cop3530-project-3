#include "menu.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

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