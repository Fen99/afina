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
	ThreadLocalPonter(T* initial, std::function<void(void*)> destructor) {
		ASSERT_INT_FUNCTION_FOR_ZERO(pthread_key_create(&_key, destructor.target<void(void*)>()), _key = 0);
		set(initial);
	}

	inline T* get() const {
		return static_cast<T*>(pthread_getspecific(_key));
	}

	inline void set(T* val) {
		ASSERT_INT_FUNCTION_FOR_ZERO(pthread_setspecific(_key, (void*) val));
	}

	T& operator*() const { 
		return *get();
	}

private:
	pthread_key_t _key;
};

} //namespace Core
} //namespace Afina

#endif // AFINA_THREAD_LOCAL_POINTER_HPP
