#include "inverted_index.h"
#include "tokenizer.h"
#include <algorithm>
#include <set>

// Build the inverted index from a collection of documents.
// For each token in each document, store the document ID and token position.
void InvertedIndex::buildIndex(const std::vector<Document>& docs) {
    index.clear();
    for (const Document& doc : docs) {
        std::vector<std::string> tokens = tokenize(doc.content);
        for (size_t pos = 0; pos < tokens.size(); ++pos)
            index[tokens[pos]].push_back({doc.id, static_cast<int>(pos)});
    }
}

// Search for documents containing a specific keyword (case-insensitive, punctuation removed).
// Returns a vector of document IDs where the keyword appears.
std::vector<int> InvertedIndex::searchKeyword(const std::string& word) {
    std::string processed = word;
    toLower(processed);
    removePunctuation(processed);
    std::set<int> docIds;
    if (index.find(processed) != index.end())
        for (const auto& entry : index[processed])
            docIds.insert(entry.first);
    return std::vector<int>(docIds.begin(), docIds.end());
}

// Search for documents containing an exact phrase.
// Returns a vector of document IDs where the phrase appears in order and consecutively.
std::vector<int> InvertedIndex::searchPhrase(const std::string& phrase) {
    std::vector<std::string> tokens = tokenize(phrase);
    if (tokens.empty()) return {};
    std::vector<int> candidates = searchKeyword(tokens[0]);
    std::vector<int> results;
    for (int docId : candidates) {
        std::vector<int> positions;
        // Find all positions of the first token in this document
        for (const auto& entry : index[tokens[0]])
            if (entry.first == docId) positions.push_back(entry.second);
        // For each position, check if the subsequent tokens appear consecutively
        for (int pos : positions) {
            bool match = true;
            for (size_t i = 1; i < tokens.size(); ++i) {
                int nextPos = pos + static_cast<int>(i);
                auto it = std::find_if(index[tokens[i]].begin(), index[tokens[i]].end(),
                    [docId, nextPos](const std::pair<int, int>& p) {
                        return p.first == docId && p.second == nextPos;
                    });
                if (it == index[tokens[i]].end()) { match = false; break; }
            }
            if (match) { results.push_back(docId); break; }
        }
    }
    return results;
}

// Clear the inverted index.
void InvertedIndex::clear() { index.clear(); }
