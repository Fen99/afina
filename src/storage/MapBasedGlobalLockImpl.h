#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <mutex>

#include "MapBasedImplementation.h"
#include <afina/core/Debug.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */
	
class MapBasedGlobalLockImpl : public MapBasedImplementation {
public:
	//max_size - in bytes
	MapBasedGlobalLockImpl(size_t max_size = std::numeric_limits<int>::max());
	virtual ~MapBasedGlobalLockImpl();

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
	std::mutex _map_mutex;
};

} // namespace Backend
} // namespace Afina


#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
