#ifndef SUFFIXARRAY_H
#define SUFFIXARRAY_H
#include <string>
#include <vector>
#include "document.h"
using namespace std;

class SuffixArray {
    vector<Document> text;
    vector<vector<int>> suffixArray;
    vector<int> constructSuffixArray(const string& text);
    vector<int> searchInDocument(const string& text, const vector<int>& suffixArray, const string& query) const;
public:
    void buildIndex(const vector<Document>& docs);
    vector<int> searchKeyword(const string& keyword) const;
    vector<int> searchPhrase(const string& phrase) const;
    void clear();
};

#endif
