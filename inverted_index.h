#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include <vector>
#include <string>
#include <unordered_map>
#include "document.h"

// InvertedIndex class builds and manages an inverted index for fast keyword and phrase searches.
class InvertedIndex {
    // The index maps each token (word) to a vector of (document ID, token position) pairs.
    std::unordered_map<std::string, std::vector<std::pair<int, int>>> index;
public:
    // Build the inverted index from a vector of Document objects.
    void buildIndex(const std::vector<Document>& docs);

    // Search for documents containing the given keyword.
    // Returns a vector of document IDs where the keyword appears.
    std::vector<int> searchKeyword(const std::string& word);

    // Search for documents containing the given phrase (exact consecutive tokens).
    // Returns a vector of document IDs where the phrase appears.
    std::vector<int> searchPhrase(const std::string& phrase);

    // Clear the inverted index.
    void clear();
};

#endif // INVERTED_INDEX_H
