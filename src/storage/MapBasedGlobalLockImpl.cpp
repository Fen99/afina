#include "MapBasedGlobalLockImpl.h"

#include <mutex>
#include <iostream>

namespace Afina {
namespace Backend {

MapBasedGlobalLockImpl::MapBasedGlobalLockImpl(size_t max_size) : MapBasedImplementation(max_size)
{}

MapBasedGlobalLockImpl::~MapBasedGlobalLockImpl()
{
	std::lock_guard<std::mutex> __lock(_map_mutex);
	Clear();
}


// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value)
{
	if (_GetElementSize(key, value) > GetMaxSize()) { return false; }

	std::lock_guard<std::mutex> __lock(_map_mutex);
	return MapBasedImplementation::Put(key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value)
{
	if (_GetElementSize(key, value) > GetMaxSize()) { return false; }

	std::lock_guard<std::mutex> __lock(_map_mutex);
	return MapBasedImplementation::PutIfAbsent(key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value)
{ 
	if (_GetElementSize(key, value) > GetMaxSize()) { return false; }

	std::lock_guard<std::mutex> __lock(_map_mutex);
	return MapBasedImplementation::Set(key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key)
{ 
	std::lock_guard<std::mutex> __lock(_map_mutex);
	return MapBasedImplementation::Delete(key);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value)
{
	std::lock_guard<std::mutex> __lock(_map_mutex);
	return MapBasedImplementation::Get(key, value);
}

void MapBasedGlobalLockImpl::Print()
{
	std::lock_guard<std::mutex> __lock(_map_mutex);
	MapBasedImplementation::Print();
}

} // namespace Backend
} // namespace Afina
