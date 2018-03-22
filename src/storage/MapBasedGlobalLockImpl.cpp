#include "MapBasedGlobalLockImpl.h"

#include <mutex>
#include <iostream>

#define LOCK_MAP_MUTEX std::lock_guard<std::recursive_mutex> lock(_map_mutex)

namespace Afina {
namespace Backend {

MapBasedGlobalLockImpl::MapBasedGlobalLockImpl(size_t max_size) :
	_max_size(max_size), _current_size(0), _backend(), _list(), _map_mutex()
{}

MapBasedGlobalLockImpl::~MapBasedGlobalLockImpl()
{}

void MapBasedGlobalLockImpl::_DeleteToSize(size_t size)
{
	LOCK_MAP_MUTEX;

	while (_current_size > size && !_list.is_empty())
	{
		auto current_element = _list.rcstart();
		_current_size -= (*current_element).GetSize();

		_backend.erase((*current_element).key);
		_list.remove(current_element);
	}
}

bool MapBasedGlobalLockImpl::_Insert(const std::string &key, const std::string &value, bool need_replace)
{
	LOCK_MAP_MUTEX;

	Data new_element = {key, value};
	if (new_element.GetSize() > _max_size) { return false; }

	auto position = _backend.find(key);
	if (position == _backend.end())
	{
		if (new_element.GetSize() + _current_size > _max_size)	{ _DeleteToSize(_max_size - new_element.GetSize()); }
		ListConstIterator it = _list.push_front({key, value});
		_backend.emplace((*it).key, it);

		_current_size += new_element.GetSize();
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
	LOCK_MAP_MUTEX;

	auto position = _backend.find(key);
	if (position == _backend.end()) { return false; }

	Data new_element = {key, value};
	if (new_element.GetSize() > _max_size) { return false; }

	int delta = (int) ((int) new_element.GetSize() - (int) (*(position->second)).GetSize());
	if (delta > (int) _max_size - _current_size) { _DeleteToSize(_max_size - delta); }

	ListIterator it = _list.remove_constness(position->second);
	(*it).value = value;
	_current_size += delta;

	_list.to_head(position->second);

	return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key)
{ 
	LOCK_MAP_MUTEX;

	auto position = _backend.find(key);
	if (position == _backend.end()) { return false; }
	ListConstIterator it = position->second;

	_current_size -= (*it).GetSize();

	_backend.erase(key);
	_list.remove(it);

	return true; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value)
{
	LOCK_MAP_MUTEX;

	auto position = _backend.find(key);
	if (position == _backend.end()) { return false; }

	value = (*(position->second)).value;
	_list.to_head(position->second);

	return true; 
}

void MapBasedGlobalLockImpl::Print()
{
	std::cout << "List printing: " << std::endl;
	_list.PrintList();

	std::cout << "Map printing: " << std::endl;
	for (auto it = _backend.cbegin(); it != _backend.cend(); it++)
	{
		std::cout << "| key = " <<  *(it->first) << " | value = key(" << (*(it->second)).key << ")+value(" << (*(it->second)).value << ") |" << std::endl;
	}
}

} // namespace Backend
} // namespace Afinua
