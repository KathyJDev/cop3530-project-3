#include "suffix_array.h"
#include "tokenizer.h"
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <cctype>
using namespace std;

// Builds the suffix array for each document
// For each document, it constructs the suffix array and stores it
void SuffixArray::buildIndex(const vector<Document>& docs) {
    text.clear();
    suffixArray.clear();
    for (const auto& doc : docs) {
        Document temp = doc;
        toLower(temp.content);
        text.push_back(temp);
        suffixArray.push_back(constructSuffixArray(temp.content));
    }
}

// Constructs the suffix array for a single text string
vector<int> SuffixArray::constructSuffixArray(const string& text) {
    int n = text.size();
    vector<int> sa(n);

    // Initialize suffix array with indices
    for (int i = 0; i < n; i++) {
        sa[i] = i;
    }

    // Sort suffixes in lexicographical order
    sort(sa.begin(), sa.end(), [&text](int i, int j) {
        return text.compare(i, text.size() - i, text, j, text.size() - j) < 0;
    });
    return sa;
}

// Searches for the keyword in all documents
// Returns a vector of document IDs where the keyword appears
vector<int> SuffixArray::searchKeyword(const string& keyword) const {
    string processed = keyword;
    toLower(processed);
    vector<int> result;
    for (int i = 0; i < text.size(); i++) {
        auto pos = searchInDocument(text[i].content, suffixArray[i], processed);
        if (!pos.empty()) {
            result.push_back(text[i].id);
        }
    }
    return result;
}

// Searches for the phrase in all documents
// Returns a vector of document IDs where the phrase appears
vector<int> SuffixArray::searchPhrase(const string& phrase) const {
    string processed = phrase;
    toLower(processed);
    vector<int> result;
    for (int i = 0; i < text.size(); i++) {
        auto pos = searchInDocument(text[i].content, suffixArray[i], processed);
        if (!pos.empty()) {
            result.push_back(text[i].id);
        }
    }
    return result;
}

// Helper function to check if a character is a word boundary.
// A boundary is anything that is NOT a letter or a number.
bool isBoundary(char c) {
    return !std::isalnum(static_cast<unsigned char>(c));
}

// Searches for the keyword/phrase (query) in the suffix array of one document
// Returns a vector of suffix array indices where the query matches
// Searches for the query as a WHOLE WORD in the suffix array of one document
std::vector<int> SuffixArray::searchInDocument(const std::string& text, const std::vector<int>& suffixArray, const std::string& query) const {
    int left = 0, right = suffixArray.size() - 1;
    int mid;
    int first_match = -1;
    std::vector<int> result;

    // 1. Binary search to find the FIRST occurrence of the query as a prefix
    while (left <= right) {
        mid = left + (right - left) / 2;
        int compare = text.compare(suffixArray[mid], query.size(), query);
        if (compare == 0) {
            first_match = mid;
            right = mid - 1; // Keep looking left to find the very first match
        } else if (compare < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    // 2. If a match was found, iterate through all matches and validate them
    if (first_match != -1) {
        for (int i = first_match; i < suffixArray.size(); ++i) {
            // Check if the suffix still starts with the query
            if (text.compare(suffixArray[i], query.size(), query) != 0) {
                break; // We've passed all potential matches
            }

            int match_pos = suffixArray[i];

            // --- BOUNDARY CHECK ---
            // Check the character BEFORE the match
            bool left_boundary_ok = (match_pos == 0) || isBoundary(text[match_pos - 1]);
            
            // Check the character AFTER the match
            bool right_boundary_ok = (match_pos + query.size() == text.size()) || isBoundary(text[match_pos + query.size()]);

            if (left_boundary_ok && right_boundary_ok) {
                result.push_back(match_pos);
            }
        }
    }
    return result;
}

bool SuffixArray::save(const std::string& filename) const {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        return false; // Failed to open file
    }

    // 1. Write the number of documents
    size_t num_docs = text.size();
    ofs.write(reinterpret_cast<const char*>(&num_docs), sizeof(num_docs));

    // 2. Loop through each document and its suffix array
    for (size_t i = 0; i < num_docs; ++i) {
        // --- Save the Document object ---
        // a. Save the ID
        ofs.write(reinterpret_cast<const char*>(&text[i].id), sizeof(text[i].id));
        
        // b. Save the content string (length-prefixed)
        size_t content_len = text[i].content.size();
        ofs.write(reinterpret_cast<const char*>(&content_len), sizeof(content_len));
        ofs.write(text[i].content.c_str(), content_len);

        // --- Save the corresponding suffix array ---
        // a. Save the size of the vector
        size_t sa_size = suffixArray[i].size();
        ofs.write(reinterpret_cast<const char*>(&sa_size), sizeof(sa_size));
        
        // b. Save the vector's raw data
        ofs.write(reinterpret_cast<const char*>(suffixArray[i].data()), sa_size * sizeof(int));
    }

    return true;
}

// Loads the index from a binary file
bool SuffixArray::load(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        return false; // Failed to open file
    }

    clear(); // Clear any existing data

    size_t num_docs;
    ifs.read(reinterpret_cast<char*>(&num_docs), sizeof(num_docs));

    // Pre-allocate memory for efficiency without calling the default constructor
    text.reserve(num_docs);
    suffixArray.reserve(num_docs);

    // Loop to read each document and its suffix array one by one
    for (size_t i = 0; i < num_docs; ++i) {
        // --- Load Document data into temporary variables ---
        int current_id;
        ifs.read(reinterpret_cast<char*>(&current_id), sizeof(current_id));

        size_t content_len;
        ifs.read(reinterpret_cast<char*>(&content_len), sizeof(content_len));
        std::string current_content(content_len, '\0');
        ifs.read(&current_content[0], content_len);

        // --- Construct the Document and add it to the vector ---
        text.emplace_back(current_id, current_content);

        // --- Load the corresponding suffix array ---
        size_t sa_size;
        ifs.read(reinterpret_cast<char*>(&sa_size), sizeof(sa_size));
        std::vector<int> current_sa(sa_size);
        ifs.read(reinterpret_cast<char*>(current_sa.data()), sa_size * sizeof(int));
        
        suffixArray.push_back(current_sa);
    }

    return true;
}

// Clear the suffix array and document text
void SuffixArray::clear() {
    suffixArray.clear();
    text.clear();
}