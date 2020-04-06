#ifndef AFINA_STORAGE_MAP_IMPLEMENTATION_H
#define AFINA_STORAGE_MAP_IMPLEMENTATION_H

#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>

#include <afina/Storage.h>
#include <afina/core/Debug.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 *
 *
 */

using StrCRef = std::reference_wrapper<const std::string>;

struct StringReferenceHash {
    size_t operator()(const StrCRef &str) const { return std::hash<std::string>()(str); }
};

struct StringReferenceEqual {
    bool operator()(const StrCRef &str1, const StrCRef &str2) const { return str1.get() == str2.get(); }
};

class MapBasedImplementation : public Afina::Storage {
protected:
    static size_t _GetElementSize(const std::string &key, const std::string &value) {
        return key.size() + value.size();
    }

private:
    struct Entry {
        std::string key;
        std::string value;

        Entry *next;
        Entry *previous;

        Entry(const std::string &key_p, const std::string &value_p, Entry *previous_p, Entry *next_p)
            : key(key_p), value(value_p), next(next_p), previous(previous_p) {}
        size_t GetSize() const { return _GetElementSize(key, value); }
    };

protected:
    MapBasedImplementation(size_t max_size = std::numeric_limits<int>::max());
    virtual ~MapBasedImplementation();

    // Implements Afina::Storage interface
    virtual bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    virtual bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    virtual bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    virtual bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    virtual bool Get(const std::string &key, std::string &value) override;

    void Print();

    size_t GetMaxSize() const { return _max_size; }
    size_t GetCurrentSize() const { return _current_size; }

    void Clear();

private:
    size_t _current_size;
    size_t _max_size;

    Entry *_first;
    Entry *_last;

    std::unordered_map<StrCRef, const Entry *, StringReferenceHash, StringReferenceEqual> _backend;

private:
    bool _Insert(const std::string &key, const std::string &value, bool need_replace);

    // Deletes elements until _current_size >= size
    void _ShrinkToSize(size_t size);

    void _MoveToHead(Entry *entry);

    // calls delete entry
    void _RemoveFromList(const Entry *entry);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_IMPLEMENTATION_H
