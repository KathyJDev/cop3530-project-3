#include "tokenizer.h"
#include <algorithm>
#include <cctype>
#include <sstream>

// Converts all characters in the string to lowercase (in place)
void toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c){ return std::tolower(c); });
}

// Removes all punctuation characters from the string (in place)
void removePunctuation(std::string& s) {
    s.erase(std::remove_if(s.begin(), s.end(),
        [](unsigned char c){ return std::ispunct(c); }), s.end());
}

// Splits the input text into lowercase, punctuation-free tokens (words)
// Returns a vector of tokens (words)
std::vector<std::string> tokenize(const std::string& text) {
    std::string processed = text;
    toLower(processed);           // Convert to lowercase
    removePunctuation(processed); // Remove punctuation

    std::vector<std::string> tokens;
    std::istringstream iss(processed);
    std::string token;
    // Extract words separated by whitespace
    while (iss >> token) tokens.push_back(token);
    return tokens;
}
