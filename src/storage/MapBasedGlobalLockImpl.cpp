#include "MapBasedGlobalLockImpl.h"

#include <mutex>
#include <iostream>

namespace Afina {
namespace Backend {

MapBasedGlobalLockImpl::MapBasedGlobalLockImpl(size_t max_size) :
	_max_size(max_size), _current_size(0), _backend(), _first(nullptr), _last(nullptr)
{}

MapBasedGlobalLockImpl::~MapBasedGlobalLockImpl()
{
	std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

	while (_first != nullptr)
	{
		Entry* current_elemtent = _first;
		_RemoveFromList(_first);
	}

	//_backend will clear itself
}

void MapBasedGlobalLockImpl::_ShrinkToSize(size_t size)
{
	std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

	while (_current_size > size && _last != nullptr)
	{
		Entry* current_element = _last;
		_backend.erase(current_element->key);
		_RemoveFromList(_last);
	}
}

bool MapBasedGlobalLockImpl::_Insert(const std::string &key, const std::string &value, bool need_replace)
{
	int size_new = _GetElementSize(key, value);
	if (size_new > _max_size) { return false; }
	std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

	auto position = _backend.find(key);
	if (position == _backend.end())
	{
		Entry* new_element = new Entry(key, value, _first, nullptr);
		if (size_new + _current_size > _max_size)	{ _ShrinkToSize(_max_size - size_new); }
		_first = new_element;
		_backend.emplace(new_element->key, new_element);

		_current_size += size_new;
	}
	else
	{
		if (need_replace == false) { return false; }
		return Set(key, value);
	}

	return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value)
{
	return _Insert(key, value, true);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value)
{
	return _Insert(key, value, false);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value)
{ 
	int size_new = _GetElementSize(key, value);

	if (size_new > _max_size) { return false; }
	std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

	auto position = _backend.find(key);
	if (position == _backend.end()) { return false; }

	int delta = size_new - position->second->GetSize();
	if (delta > (int) (_max_size - _current_size)) { _ShrinkToSize(_max_size - delta); }
	position = _backend.find(key);
	if (position == _backend.end()) { return _Insert(key, value, false); } //If element was delited during clearing of a storage (_DeleteToSize) 

	Entry* current_element = const_cast<Entry*>(position->second);
	current_element->value = value;
	_current_size += delta;

	_MoveToHead(current_element);
	
	return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key)
{ 
	std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

	auto position = _backend.find(key);
	if (position == _backend.end()) { return false; }
	Entry* current_element = const_cast<Entry*>(position->second);

	_backend.erase(key);
	_RemoveFromList(current_element);

	return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value)
{
	std::lock_guard<std::recursive_mutex> __lock(_map_mutex);

	auto position = _backend.find(key);
	if (position == _backend.end()) { return false; }

	value = position->second->value;
	_MoveToHead(const_cast<Entry*>(position->second));

	return true; 
}

void MapBasedGlobalLockImpl::Print()
{
	std::cout << "List printing: " << std::endl;
	Entry* element = _first;
	while (element != nullptr)
	{
		std::cout << "Key: " << element->key << "; value = " << element->value <<
					 "; previous = " << element->previous << "; next = " << element->next << std::endl;
		element = element->next;
	}

	std::cout << "Map printing: " << std::endl;
	for (auto it = _backend.cbegin(); it != _backend.cend(); it++)
	{
		std::cout << "key = " <<  it->first << " | value = const Entry*(" << it->second << ")" << std::endl;
	}
}

void MapBasedGlobalLockImpl::_MoveToHead(Entry* entry)
{
	Entry* previous = entry->previous;
	Entry* next = entry->next;
	if (previous != nullptr) { previous->next = next; }
	if (next != nullptr)     { next->previous = previous; }

	entry->next = _first;
	_first->previous = entry;
}

void MapBasedGlobalLockImpl::_RemoveFromList(const Entry* entry)
{
	Entry* previous = entry->previous;
	Entry* next = entry->next;
	if (previous != nullptr) { previous->next = next; }
	if (next != nullptr)     { next->previous = previous; }

	_current_size -= entry->GetSize();
	delete const_cast<Entry*>(entry);
}

} // namespace Backend
} // namespace Afina
