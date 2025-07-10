#include "suffix_array.h"
#include <algorithm>
#include <vector>
#include <string>
using namespace std;

void SuffixArray::clear() {
    suffixArray.clear();
    text.clear();
}

// Builds the suffix array for each document
void SuffixArray::buildIndex(const vector<Document>& docs) {
    text = docs;
    suffixArray.clear();
    for (const auto& doc : docs) {
        suffixArray.push_back(constructSuffixArray(doc.content));
    }
}

// Constructs the suffix array for one text
vector<int> SuffixArray::constructSuffixArray(const string& text) {
    int n = text.size();
    vector<int> sa(n);
    for (int i = 0; i < n; i++) {
        sa[i] = i;
    }
    sort(sa.begin(), sa.end(), [&text](int i, int j) {
        return text.substr(i) < text.substr(j);
    });
    return sa;
}

// Searches for the query in all documents
vector<int> SuffixArray::search(const string& query) const {
    vector<int> result;
    for (int i = 0; i < text.size(); i++) {
        auto pos = searchInDocument(text[i].content, suffixArray[i], query);
        if (!pos.empty()) {
            result.push_back(text[i].id);
        }
    }
    return result;
}

// Searches for the query in the suffix array of one document
vector<int> SuffixArray::searchInDocument(const string& text, const vector<int>& suffixArray, const string& query) const {
    int left = 0, right = suffixArray.size() - 1;
    int mid;
    string suffix;
    vector<int> result;

    // Binary search for the query
    while (left <= right) {
        mid = left + (right - left) / 2;
        suffix = text.substr(suffixArray[mid], query.size());
        if (suffix == query) {
            int temp = mid;

            // Finds all suffixes that match the query
            while (temp >= left && text.substr(suffixArray[temp], query.size()) == query) {
                result.push_back(suffixArray[temp]);
                temp--;
            }
            temp = mid + 1;
            while (temp <= right && text.substr(suffixArray[temp], query.size()) == query) {
                result.push_back(suffixArray[temp]);
                temp++;
            }
            break;
        } else if (suffix < query) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}