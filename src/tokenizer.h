#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <vector>
#include <string>

// Converts all characters in the given string to lowercase (in place)
void toLower(std::string& s);

// Removes all punctuation characters from the given string (in place)
void removePunctuation(std::string& s);

// Tokenizes the input text: converts to lowercase, removes punctuation, and splits into words
// Returns a vector of tokens (words)
std::vector<std::string> tokenize(const std::string& text);

#endif // TOKENIZER_H
