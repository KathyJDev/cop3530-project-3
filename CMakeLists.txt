cmake_minimum_required(VERSION 3.22)
project(cop3530_project_3)

set(CMAKE_CXX_STANDARD 14)

#compile flags to match Gradescope test environment
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

##pull in desired version of catch through cmake automatically, make it available
Include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2 # if you run into issues, try updating your editor or downgrading to a lower version of catch v3
)
FetchContent_MakeAvailable(Catch2)

add_executable(search_engine
        main.cpp
        document.cpp
        document.h
        suffix_array.cpp
        suffix_array.h
        inverted_index.cpp
        inverted_index.h
        tokenizer.cpp
        tokenizer.h
        utils.h
        utils.cpp
        performance.cpp
        performance.h
)

# These tests can use the Catch2-provided main
add_executable(tests
        test_inverted_index.cpp
        test_tokenizer.cpp
        test_suffix_array.cpp
        suffix_array.h
        suffix_array.cpp
        test_tokenizer.cpp
        document.cpp
        document.h
        inverted_index.h
        inverted_index.cpp
        tokenizer.h
        tokenizer.cpp
        utils.cpp
        utils.h
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain) #link catch to test.cpp file
# the name here must match that of your testing executable (the one that has test.cpp)

# comment everything below out if you are using CLion