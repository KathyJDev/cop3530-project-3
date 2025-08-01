#ifndef CUSTOM_HASH_TABLE_H
#define CUSTOM_HASH_TABLE_H

#include <vector>
#include <list>
#include <string>
#include <type_traits> // For std::is_same

template <typename K, typename V>
class HashTable {
private:
    /**
     * @struct Entry
     * @brief A simple key-value pair used for entries in the hash table buckets.
     */
    struct Entry {
        K key;
        V value;
    };

    // The main storage for the hash table. It's a vector of buckets,
    // where each bucket is a linked list of Entries (separate chaining).
    std::vector<std::list<Entry>> table;

    // Tracks the total number of key-value pairs currently in the table.
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
    
    /**
     * @brief Resizes the hash table to double its capacity.
     * This function is called when the load factor exceeds a certain threshold.
     * It re-hashes and re-inserts all existing elements into the new, larger table.
     */
    void rehash() {
        // Keep the old table to iterate over it
        std::vector<std::list<Entry>> oldTable = std::move(table);

        // Create a new table with double the capacity and reset the size.
        // The size will be restored as we re-insert.
        table.assign(oldTable.size() * 2, std::list<Entry>());
        currentSize = 0; // Temporarily reset size

        // Directly re-insert all elements from the old table
        for (const auto& bucket : oldTable) {
            for (const auto& entry : bucket) {
                // This logic is self-contained and does NOT call operator[] to avoid recursion
                size_t newBucketIndex = hashFunction(entry.key);
                table[newBucketIndex].push_back(entry);
                currentSize++; // Increment size for each re-inserted element
            }
        }
    }

public:
    /**
     * @brief Constructs the hash table.
     * @param initialSize The initial number of buckets for the table.
     */
    explicit HashTable(size_t initialSize = 16) : currentSize(0) {
        if (initialSize == 0) initialSize = 16;
        table.resize(initialSize);
    }
    
    /**
     * @brief Removes all key-value pairs from the hash table.
     */
    void clear() {
        for (auto& bucket : table) {
            bucket.clear();
        }
        currentSize = 0;
    }

    /**
     * @brief Accesses or inserts an element.
     * If the key exists, returns a reference to its value.
     * If the key does not exist, it is inserted with a default-constructed
     * value, and a reference to this new value is returned.
     * Triggers a rehash if the load factor (0.75) is exceeded before insertion.
     * @param key The key of the element to access.
     * @return A reference to the value corresponding to the key.
     */
    V& operator[](const K& key) {
        if (currentSize >= table.size() * 0.75) {
            rehash();
        }

        size_t bucketIndex = hashFunction(key);
        auto& bucket = table[bucketIndex];

        // Search for existing key
        for (auto& entry : bucket) {
            if (entry.key == key) {
                return entry.value;
            }
        }

        // Key not found, insert new element
        bucket.push_back({key, V{}});
        currentSize++;
        return bucket.back().value;
    }

    /**
     * @brief Searches for an element with a given key.
     * @param key The key to search for.
     * @return A pointer to the value if the key is found, otherwise nullptr.
     */
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

    /**
     * @brief Retrieves all entries from the hash table.
     * A helper method for serialization that flattens the internal table
     * structure into a single vector of all key-value pairs.
     * @return A std::vector containing all Entry objects.
     */
    std::vector<Entry> get_all_entries() const {
        std::vector<Entry> entries;
        entries.reserve(currentSize);
        for (const auto& bucket : table) {
            for (const auto& entry : bucket) {
                entries.push_back(entry);
            }
        }
        return entries;
    }
};

#endif // CUSTOM_HASH_TABLE_H