#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <unordered_map>
#include <mutex>
#include <string>
#include <functional>
#include <memory>
#include <iostream>
#include <limits>

#include <afina/Storage.h>
#include "../core/Debug.h"

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */

// class StringPointerWrapper {
// 	private:
// 		const std::string* _str;
// 
// 	public:
// 		class Hash {
// 			public:
// 				size_t operator() (const StringPointerWrapper& str) const
// 				{
// 					return std::hash<std::string>{}(*str._str);
// 				}
// 		};
// 
// 	public:
// 		StringPointerWrapper(const std::string& str) : _str(&str) {}
// 
// 		const std::string& operator*() const { return *_str; }
// 		bool operator<  (const StringPointerWrapper& second) const { return *_str <  *second._str; }
// 		bool operator== (const StringPointerWrapper& second) const { return *_str == *second._str; }
// };

class MapBasedGlobalLockImpl : public Afina::Storage {
private:
	static size_t _GetElementSize(const std::string& key, const std::string& value) { return key.size() + value.size(); }

	struct Entry
	{
		std::string key;
		std::string value;

		Entry* next;
		Entry* previous;

		Entry(const std::string& key_p, const std::string& value_p, Entry* next_p, Entry* previous_p) : key(key_p), value(value_p), next(next_p),
																										previous(previous_p) {}
		size_t GetSize() const { return _GetElementSize(key, value); }
	};

public:
	//max_size - in bytes, -1 = unlimited
	MapBasedGlobalLockImpl(size_t max_size = std::numeric_limits<int>::max());
	~MapBasedGlobalLockImpl();

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

    void Print();

private:
    size_t _max_size;
	size_t _current_size;

	Entry* _first;
	Entry* _last;

	std::unordered_map<std::cref<std::string>, const Entry*> _backend;

	std::recursive_mutex _map_mutex;

private:
	bool _Insert(const std::string &key, const std::string &value, bool need_replace);
	
	// Deletes elements until _current_size >= size
	void _ShrinkToSize(size_t size);

	void _MoveToHead(Entry* entry);

	//calls delete entry
	void _RemoveFromList(const Entry* entry);
};

} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
