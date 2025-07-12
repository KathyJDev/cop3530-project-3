#include <catch2/catch_all.hpp>
#include <iostream>
#include "inverted_index.h"

using namespace std;

TEST_CASE("Test Cases for inverted_index.cpp", "[inverted_index.h]"){
  InvertedIndex invertedIndex;
  vector<Document> docs = {
    {1, "Google is an American search engine company, founded in 1998 by Sergey Brin and Larry Page."},
    {2, "Google Docs, Google Sheets, Google Slides, Google Draw"},
    {3, "Google purchases YouTube for 1.5 billion dollars"}
  };
  SECTION("buildIndex, searchKeyword") {
    invertedIndex.buildIndex(docs);
    REQUIRE(invertedIndex.searchKeyword("Google") == vector<int>{1,2,3});
    REQUIRE(invertedIndex.searchKeyword("google") == vector<int>{1,2,3});
    REQUIRE(invertedIndex.searchKeyword("docs") == vector<int>{2});
    REQUIRE(invertedIndex.searchKeyword("2") == vector<int>{});
    REQUIRE(invertedIndex.searchKeyword("Alphabet") == vector<int>{});
  }
  SECTION("searchPhrase") {
    invertedIndex.buildIndex(docs);
    REQUIRE(invertedIndex.searchPhrase("Google Docs") == vector<int>{2});
    REQUIRE(invertedIndex.searchPhrase("search engine") == vector<int>{1});
    REQUIRE(invertedIndex.searchPhrase("1.5 Billion") == vector<int>{3});
  }

  SECTION("clear"){
    invertedIndex.buildIndex(docs);
    REQUIRE(invertedIndex.searchKeyword("Google") == vector<int>{1,2,3});
    invertedIndex.clear();
    REQUIRE(invertedIndex.searchKeyword("Google") == vector<int>{});
  }
}