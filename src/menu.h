#ifndef MENU_H
#define MENU_H

#include <vector>
#include <string>
#include "document.h"

// Function declarations for menu-related logic
void printMenu();
int searchMethod();
int selectDocument(const std::vector<int>& docIds, const std::vector<Document>& docs);

#endif // MENU_H