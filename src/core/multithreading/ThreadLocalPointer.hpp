#ifndef AFINA_THREAD_LOCAL_POINTER_HPP
#define AFINA_THREAD_LOCAL_POINTER_HPP

#include <pthread.h>

#include <afina/core/Debug.h>

namespace Afina {
namespace Core {

template <typename T>
class ThreadLocalPonter
{
public:
	ThreadLocalPonter(T* initial = nullptr, std::function<void(T*)> destructor = nullptr) {
		ASSERT_INT_FUNCTION_FOR_ZERO(pthread_key_create(&_key, destructor), _key = 0);
		set(initial);
	}

	inline T* get() const {
		ASSERT(_key != 0);
		return pthread_getspecific(_key);
	}

	inline void set(T* val) {
		ASSERT(_key != 0);
		ASSERT_INT_FUNCTION_FOR_ZERO(pthread_setspecific(_key, val));
	}

	T& operator*() const { 
		ASSERT(_key != 0);
		return *get();
	}

private:
	pthread_key_t _key;
};

} //namespace Core
} //namespace Afina

#endif // AFINA_THREAD_LOCAL_POINTER_HPP