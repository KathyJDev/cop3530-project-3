#include "inverted_index.h"
#include "tokenizer.h"
#include <algorithm>
#include <fstream>
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
// Search for documents containing a specific keyword (UPDATED)
std::vector<int> InvertedIndex::searchKeyword(const std::string& word) {
    std::string processed = word;
    toLower(processed);
    removePunctuation(processed);
    
    std::set<int> docIds;

    // Our custom find() returns a pointer to the value, or nullptr if not found.
    // So, we check against nullptr instead of index.end().
    auto* postings = index.find(processed);
    
    if (postings != nullptr) { // This is the corrected check
        // If found, dereference the pointer to loop through the vector of postings.
        for (const auto& entry : *postings) {
            docIds.insert(entry.first);
        }
    }
    
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

bool InvertedIndex::save(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        return false;
    }

    // Use the new helper to get all data from the hash table
    auto all_entries = index.get_all_entries();

    for (const auto& table_entry : all_entries) {
        const std::string& token = table_entry.key;
        const auto& postings = table_entry.value;
        
        ofs << token << " " << postings.size();
        for (const auto& pos_pair : postings) {
            ofs << " " << pos_pair.first << " " << pos_pair.second;
        }
        ofs << "\n";
    }
    return true;
}

bool InvertedIndex::load(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        return false;
    }

    clear(); // Clear any existing index data
    std::string token;
    size_t postings_count;

    while (ifs >> token >> postings_count) {
        std::vector<std::pair<int, int>> postings(postings_count);
        for (size_t i = 0; i < postings_count; ++i) {
            ifs >> postings[i].first >> postings[i].second;
        }
        index[token] = postings;
    }
    return true;
}

// Clear the inverted index.
void InvertedIndex::clear() { index.clear(); }
