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
#include "List.h"

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */

class StringPointerWrapper {
	private:
		const std::string* _str;

	public:
		class Hash {
			public:
				size_t operator() (const StringPointerWrapper& str) const
				{
					return std::hash<std::string>{}(*str._str);
				}
		};

	public:
		StringPointerWrapper(const std::string& str) : _str(&str) {}

		const std::string& operator*() const { return *_str; }
		bool operator<  (const StringPointerWrapper& second) const { return *_str <  *second._str; }
		bool operator== (const StringPointerWrapper& second) const { return *_str == *second._str; }
};

class MapBasedGlobalLockImpl : public Afina::Storage {
private:
	struct Data
	{
		std::string key;
		std::string value;

		size_t GetSize() const { return key.size() + value.size(); }
	};

	using ListType = List<Data>;
	using ListConstIterator = ListType::const_iterator;
	using ListIterator = ListType::iterator;

public:
	//max_size - in bytes, -1 = unlimited
	MapBasedGlobalLockImpl(size_t max_size = std::numeric_limits<size_t>::max());
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
	ListType _list;

	std::unordered_map<StringPointerWrapper, ListConstIterator, StringPointerWrapper::Hash> _backend;

	std::recursive_mutex _map_mutex;

private:
	bool _Insert(const std::string &key, const std::string &value, bool need_replace);
	
	// Deletes elements until _current_size >= size
	void _DeleteToSize(size_t size);
};

} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
