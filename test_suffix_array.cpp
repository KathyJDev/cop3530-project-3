#include <catch2/catch_all.hpp>
#include <iostream>
#include "suffix_array.h"

using namespace std;

TEST_CASE("Test Cases for suffix_array.cpp", "[suffix_array]"){
    SuffixArray sa;
    vector<Document> docs = {
        {1, "Google is an American search engine company"},
        {2, "Google Docs, Google Sheets"},
        {3, "Google purchases YouTube"}
    };

    SECTION("buildIndex, searchKeyword, searchPhrase , clear") {
        sa.buildIndex(docs);

        //searchKeyword Function
        vector<int> res = sa.searchKeyword("Google");
        REQUIRE(res.size() == 3);

        res.clear();

        res = sa.searchPhrase("YouTube");
        REQUIRE(res.size() == 1);
        REQUIRE(res[0] == 3);

        //searchPhrase Function
        vector<int> phrase = sa.searchPhrase("Google is");
        REQUIRE(phrase.size() == 1);
        REQUIRE(phrase[0] == 1);
        phrase.clear();

        phrase = sa.searchPhrase("Google Docs");
        REQUIRE(phrase.size() == 1);
        REQUIRE(phrase[0] == 2);

        //clear() function
        sa.clear();
        phrase = sa.searchPhrase("Google is");
        REQUIRE(phrase.size() == 0);
    }
}