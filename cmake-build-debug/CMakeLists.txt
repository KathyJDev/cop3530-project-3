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
        ../src/main.cpp
        ../src/gui.py
        ../src/generate_summary.py
        ../src/document.cpp
        ../src/document.h
        ../src/suffix_array.cpp
        ../src/suffix_array.h
        ../src/inverted_index.cpp
        ../src/inverted_index.h
        ../src/tokenizer.cpp
        ../src/tokenizer.h
        ../src/utils.h
        ../src/utils.cpp
        ../src/performance.cpp
        ../src/performance.h
)

# These tests can use the Catch2-provided main
add_executable(tests
        ../src/test_inverted_index.cpp
        ../src/test_tokenizer.cpp
        ../src/test_suffix_array.cpp
        ../src/suffix_array.h
        ../src/suffix_array.cpp
        ../src/test_tokenizer.cpp
        ../src/document.cpp
        ../src/document.h
        ../src/inverted_index.h
        ../src/inverted_index.cpp
        ../src/tokenizer.h
        ../src/tokenizer.cpp
        ../src/utils.cpp
        ../src/utils.h
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain) #link catch to test.cpp file
# the name here must match that of your testing executable (the one that has test.cpp)

# comment everything below out if you are using CLion