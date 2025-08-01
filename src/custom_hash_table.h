#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <vector>
#include <list>
#include <string>
#include <type_traits> // For std::is_same

template <typename K, typename V>
class HashTable {
private:
    struct Entry {
        K key;
        V value;
    };

    std::vector<std::list<Entry>> table;
    size_t currentSize;

    // Manual hash function for std::string keys using the djb2 algorithm.
    size_t hashFunction(const K& key) const {
        // This implementation is specifically for std::string keys.
        static_assert(std::is_same<K, std::string>::value, "This hash function is for std::string keys only.");

        unsigned long hash = 5381; // An initial seed value
        for (char c : key) {
            // Formula: hash = (hash * 33) + c
            hash = ((hash << 5) + hash) + c;
        }
        return hash % table.size();
    }
    
    void rehash() {
        // ... (rehash function remains the same as before)
        size_t oldTableSize = table.size();
        std::vector<std::list<Entry>> oldTable = std::move(table);

        table.assign(oldTableSize * 2, std::list<Entry>());
        currentSize = 0;

        for (const auto& bucket : oldTable) {
            for (const auto& entry : bucket) {
                (*this)[entry.key] = entry.value;
            }
        }
    }

public:
    explicit HashTable(size_t initialSize = 16) : currentSize(0) {
        if (initialSize == 0) initialSize = 16;
        table.resize(initialSize);
    }
    
    // The rest of the class (operator[], find, clear, etc.) remains unchanged.
    // ...
    void clear() {
        for (auto& bucket : table) {
            bucket.clear();
        }
        currentSize = 0;
    }

    V& operator[](const K& key) {
        if (currentSize >= table.size() * 0.75) {
            rehash();
        }

        size_t bucketIndex = hashFunction(key);
        auto& bucket = table[bucketIndex];

        for (auto& entry : bucket) {
            if (entry.key == key) {
                return entry.value;
            }
        }

        bucket.push_back({key, V{}});
        currentSize++;
        return bucket.back().value;
    }

    V* find(const K& key) {
        size_t bucketIndex = hashFunction(key);
        auto& bucket = table[bucketIndex];

        for (auto& entry : bucket) {
            if (entry.key == key) {
                return &entry.value;
            }
        }
        return nullptr;
    }
};

#endif // CUSTOM_HASH_TABLE_H