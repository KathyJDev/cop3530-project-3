#include "document.h"

// Constructor for the Document class.
// Initializes a Document object with a unique id and its content.
Document::Document(int id, const std::string& content)
    : id(id), content(content) {}
