#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <string>

// The Document class represents a single text document
// with a unique identifier and its full content.
class Document {
public:
    int id;                 // Unique ID for the document
    std::string content;    // The full text content of the document
    
    // Constructor: initializes a Document with an id and its content
    Document(int id, const std::string& content);
};

#endif // DOCUMENT_H
