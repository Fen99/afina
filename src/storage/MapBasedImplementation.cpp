#include "MapBasedImplementation.h"

#include <iostream>

namespace Afina {
namespace Backend {

MapBasedImplementation::MapBasedImplementation(size_t max_size)
    : _max_size(max_size), _current_size(0), _backend(), _first(nullptr), _last(nullptr) {}

MapBasedImplementation::~MapBasedImplementation() {
    Clear();
    if (!_backend.empty()) {
        CURRENT_PROCESS_DEBUG("EXCEPTION: Storage map is not empty!");
    }
}

void MapBasedImplementation::Clear() { _ShrinkToSize(0); }

void MapBasedImplementation::_ShrinkToSize(size_t size) {
    // std::lock_guard<std::recursive_mutex> __lock(_map_mutex);
    while (_current_size > size && _last != nullptr) {
        _RemoveFromList(_last);
    }
}

bool MapBasedImplementation::_Insert(const std::string &key, const std::string &value, bool need_replace) {
    int size_new = _GetElementSize(key, value);
    if (size_new > _max_size) {
        return false;
    }
    // std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

    auto position = _backend.find(key);
    if (position == _backend.end()) {
        if (size_new + _current_size > _max_size) {
            _ShrinkToSize(_max_size - size_new);
        }
        Entry *new_element = new Entry(key, value, nullptr, _first);
        if (_first != nullptr) {
            _first->previous = new_element;
        }
        _first = new_element;
        if (_last == nullptr) {
            _last = _first;
        } // for the first element
        _backend.emplace(std::cref(new_element->key), new_element);

        _current_size += size_new;
    } else {
        if (need_replace == false) {
            return false;
        }
        return MapBasedImplementation::Set(key, value);
    }

    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedImplementation::Put(const std::string &key, const std::string &value) { return _Insert(key, value, true); }

// See MapBasedGlobalLockImpl.h
bool MapBasedImplementation::PutIfAbsent(const std::string &key, const std::string &value) {
    return _Insert(key, value, false);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedImplementation::Set(const std::string &key, const std::string &value) {
    int size_new = _GetElementSize(key, value);
    if (size_new > _max_size) {
        return false;
    }
    // std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

    auto position = _backend.find(key);
    if (position == _backend.end()) {
        return false;
    }

    int delta = size_new - position->second->GetSize();
    if (delta > (int)(_max_size - _current_size)) {
        _ShrinkToSize(_max_size - delta);
    }
    position = _backend.find(key);
    if (position == _backend.end()) {
        return _Insert(key, value, false);
    } // If element was delited during clearing of a storage (_DeleteToSize)

    Entry *current_element = const_cast<Entry *>(position->second);
    current_element->value = value;
    _current_size += delta;

    _MoveToHead(current_element);

    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedImplementation::Delete(const std::string &key) {
    // std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

    auto position = _backend.find(key);
    if (position == _backend.end()) {
        return false;
    }
    _RemoveFromList(position->second);

    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedImplementation::Get(const std::string &key, std::string &value) {
    // std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

    auto position = _backend.find(key);
    if (position == _backend.end()) {
        return false;
    }

    value = position->second->value;
    _MoveToHead(const_cast<Entry *>(position->second));

    return true;
}

void MapBasedImplementation::Print() {
    std::cout << "List printing: " << std::endl;
    Entry *element = _first;
    while (element != nullptr) {
        std::cout << element << ":=: "
                  << "Key: " << element->key << "; value = " << element->value << "; previous = " << element->previous
                  << "; next = " << element->next << std::endl;
        element = element->next;
    }

    std::cout << "Map printing: " << std::endl;
    for (auto it = _backend.cbegin(); it != _backend.cend(); it++) {
        std::cout << "key = " << it->first.get() << " | value = const Entry*(" << it->second << ")" << std::endl;
    }
}

void MapBasedImplementation::_MoveToHead(Entry *entry) {
    if (entry == _first) {
        return;
    }

    Entry *previous = entry->previous;
    Entry *next = entry->next;
    if (previous != nullptr) {
        previous->next = next;
    }
    if (next != nullptr) {
        next->previous = previous;
    }
    if (entry == _last) {
        _last = previous;
    }

    _first->previous = entry;
    entry->next = _first;
    _first = entry;
}

void MapBasedImplementation::_RemoveFromList(const Entry *entry) {
    _backend.erase(entry->key);

    Entry *previous = entry->previous;
    Entry *next = entry->next;
    if (previous != nullptr) {
        previous->next = next;
    }
    if (next != nullptr) {
        next->previous = previous;
    }

    if (entry == _first) {
        _first = next;
    }
    if (entry == _last) {
        _last = previous;
    }

    Entry *element = const_cast<Entry *>(entry);
    element->previous = nullptr;
    element->next = nullptr;

    _current_size -= entry->GetSize();
    delete element;
}

} // namespace Backend
} // namespace Afina
