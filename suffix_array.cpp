#include "suffix_array.h"
#include "tokenizer.h"
#include <algorithm>
#include <vector>
#include <string>
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

// Searches for the keyword/phrase (query) in the suffix array of one document
// Returns a vector of suffix array indices where the query matches
vector<int> SuffixArray::searchInDocument(const string& text, const vector<int>& suffixArray, const string& query) const {
    int left = 0, right = suffixArray.size() - 1;
    int mid;
    int compare;
    vector<int> result;

    // Binary search for the query
    while (left <= right) {
        mid = left + (right - left) / 2;

        // Compare the suffix at the middle with the query
        compare = text.compare(suffixArray[mid], query.size(), query);
        if (compare == 0) {
            int temp = mid;

            // Find all suffixes that match the query before the middle
            while (temp >= left && text.compare(suffixArray[temp], query.size(), query) == 0) {
                result.push_back(suffixArray[temp]);
                temp--;
            }

            // Find all suffixes that match the query after the middle
            temp = mid + 1;
            while (temp <= right && text.compare(suffixArray[temp], query.size(), query) == 0) {
                result.push_back(suffixArray[temp]);
                temp++;
            }

            break;
        } else if (compare < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

// Clear the suffix array and document text
void SuffixArray::clear() {
    suffixArray.clear();
    text.clear();
}