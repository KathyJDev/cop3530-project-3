#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include "document.h"

// Loads all .txt files from the directory and returns a vector of Document objects
std::vector<Document> loadDocuments(const std::string& dirPath);

// Generates a snippet from content around the first occurrence of query
std::string generateSnippet(const std::string& content, const std::string& query, int snippetLength = 100);

// Highlights all occurrences of keyword/phrase in text (case-insensitive, ANSI colors)
std::string highlightKeyword(const std::string& text, const std::string& keyword);

// Shows all snippets (scrollable) for each occurrence of keyword/phrase in content
void showAllSnippets(const std::string& content, const std::string& keyword, int snippetLength = 80);

#endif
