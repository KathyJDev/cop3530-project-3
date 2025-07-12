#include <catch2/catch_all.hpp>
#include <sstream>
#include <iostream>
#include "document.h"
#include "tokenizer.h"
#include "inverted_index.h"
#include "performance.h"
#include "utils.h"

using namespace std;

TEST_CASE("Test Cases for tokenizer.cpp", "[tokenizer.h]") {
    SECTION("Lowercase Conversion") {
        string input = "HELLO";
        toLower(input);
        REQUIRE(input == "hello");
    }
    SECTION("Remove Punctuation") {
        string input = "Hello World, and Bye World!";
        removePunctuation(input);
        REQUIRE(input == "Hello World and Bye World");
    }
    SECTION("tokenize") {
        string input = "HELLO this is very IMportant! I am Testing the Tokenize fUnCtIon!";
        vector<string> output = tokenize(input);
        vector<string> expected = {"hello", "this", "is", "very", "important", "i", "am", "testing",
        "the", "tokenize", "function"};
        REQUIRE(output == expected);
    }
}