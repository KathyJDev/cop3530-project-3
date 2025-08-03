#include "gutenberg.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <limits>
#include <regex>

// Helper function to clean a string to be a valid filename
std::string sanitizeFilename(std::string name) {
    // Trim leading/trailing whitespace
    name.erase(0, name.find_first_not_of(" \t\n\r"));
    name.erase(name.find_last_not_of(" \t\n\r") + 1);

    // Replace invalid filename characters (e.g., :, /, \, ?, *) with an underscore
    std::regex re(R"([<>:"/\\|?*])");
    name = std::regex_replace(name, re, "_");
    
    // Limit filename length to avoid OS issues
    if (name.length() > 100) {
        name = name.substr(0, 100);
    }
    
    return name;
}

// Helper function to extract the title from a Gutenberg text file
std::string extractTitle(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ""; // Return empty if file can't be opened
    }

    std::string line;
    std::string title;
    const std::string title_prefix = "Title: ";
    int lines_to_check = 50; // Only search the top of the file for the title

    while (lines_to_check > 0 && std::getline(file, line)) {
        // Check if the line starts with "Title: "
        if (line.rfind(title_prefix, 0) == 0) { 
            title = line.substr(title_prefix.length());
            break; // Found it, no need to look further
        }
        lines_to_check--;
    }

    file.close();

    if (!title.empty()) {
        return sanitizeFilename(title);
    }
    
    return ""; // Return empty if no title was found
}

// Function to download a book from Project Gutenberg and save it by its title
void downloadGutenbergBook() {
    std::cout << "\nEnter the path to a folder that will *contain* 'test_data/' (e.g., 'my_books').\n"
                 "The 'test_data/' folder will be created inside this path to hold the Gutenberg books.\n"
                 "Press Enter to use the current directory '.' as the container folder (resulting in 'test_data/', the provided folder): ";
    std::string folderPath;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    // Use std::getline to read the entire line, allowing for empty input
    std::getline(std::cin, folderPath);

    // If the input path is empty, default it to the current directory ('.')
    if (folderPath.empty()) {
        folderPath = ".";
    }

    // storePath will be "data/test_data" if folderPath is "data"
    std::string storePath = folderPath + "/test_data";
#ifdef _WIN32
    std::string mkdir_cmd = "if not exist \"" + storePath + "\" mkdir \"" + storePath + "\" > nul 2>&1";
#else
    std::string mkdir_cmd = "mkdir -p \"" + storePath + "\"";
#endif
    // It's good practice to check the result of system()
    int mkdir_result = system(mkdir_cmd.c_str());
    if (mkdir_result != 0) {
        std::cerr << "Error: Failed to create or access directory '" << storePath << "'. Please check permissions or path.\n";
        return; // Exit if directory cannot be created/accessed
    }

    std::cout << "Enter the Project Gutenberg Book ID (e.g., 84 for Frankenstein): ";
    int bookId;
    std::cin >> bookId;

    if (std::cin.fail() || bookId <= 0) {
        std::cout << "Invalid Book ID. Please enter a positive number.\n";
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        return;
    }
    std::cin.ignore(); // Consume the newline character left by previous std::cin or other inputs

    // Download to a temporary file first
    std::string temp_file_path = storePath + "/temp_book_download.txt"; // e.g., "data/test_data/temp_book_download.txt"
    std::string url = "https://www.gutenberg.org/cache/epub/" + std::to_string(bookId) + "/pg" + std::to_string(bookId) + ".txt";
    std::string command = "curl -L -o \"" + temp_file_path + "\" \"" + url + "\"";

    std::cout << "\nDownloading book " << bookId << " from " << url << "...\n";
    int result = system(command.c_str());

    if (result != 0) {
        std::cout << "\nFailed to download the book. Make sure 'curl' is installed and in your PATH, and the Book ID is correct.\n";
        remove(temp_file_path.c_str()); // Clean up temp file on failure
        return;
    }

    // Extract title to determine the final filename
    std::string title = extractTitle(temp_file_path);
    std::string final_filename;

    if (!title.empty()) {
        final_filename = title + ".txt";
    } else {
        std::cout << "Could not automatically find a title. Using default name.\n";
        final_filename = "pg" + std::to_string(bookId) + ".txt";
    }

    std::string final_filepath = storePath + "/" + final_filename; // e.g., "data/test_data/Frankenstein.txt"

    // 1. Read content from the TEMPORARY file (this is where the downloaded data is)
    std::ifstream temp_file_in(temp_file_path);
    if (!temp_file_in.is_open()) {
        std::cerr << "Error: Could not open temporary downloaded file for reading: " << temp_file_path << "\n";
        // Do not remove temp_file_path here, it implies it couldn't even be opened for reading.
        return;
    }
    std::string content((std::istreambuf_iterator<char>(temp_file_in)), std::istreambuf_iterator<char>());
    temp_file_in.close(); // Close the temp file after reading its content

    // 2. Prepend the filename (title) to the content
    content = final_filename + "\n\n" + content;

    // 3. Write the modified content to the FINAL destination file
    std::ofstream final_file_out(final_filepath);
    if (!final_file_out.is_open()) {
        std::cerr << "Error: Could not open final destination file for writing: " << final_filepath << "\n";
        remove(temp_file_path.c_str()); // Clean up temp file since we can't write final
        return;
    }
    final_file_out << content;
    final_file_out.close(); // Close the final file after writing

    // 4. Clean up the temporary file (no need for std::rename anymore, as content was already written to final_filepath)
    remove(temp_file_path.c_str());

    std::cout << "Successfully downloaded and saved as '" << final_filename << "' in '" << storePath << "'.\n"
              << "Please re-index your documents (Option 1) to include this new book.\n";
}