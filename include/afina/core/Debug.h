#ifndef AFINA_DEBUG_H
#define AFINA_DEBUG_H

#include <iostream>
#include <cstring>

#include <pthread.h>

#define ASSERT(X) if(!(X)) { CURRENT_PROCESS_DEBUG("EXCEPTION: " << #X); throw std::runtime_error(#X); }
#define ASSERT_INT_FUNCTION_FOR_ZERO(X, ...) int __result = X; if (__result != 0) {__VA_ARGS__; CURRENT_PROCESS_DEBUG(#X << " failed with code " << __result); ASSERT(__result == 0);}

#define PROCESS_DEBUG(PID, MESSAGE) std::cout << "Process PID = " << PID << ": " << MESSAGE << std::endl
#define CURRENT_PROCESS_DEBUG(MESSAGE) PROCESS_DEBUG(pthread_self(), MESSAGE)

//Macroses for check values after system callings (if errno was set)
#define VALIDATE_CONDITION(X) if(!(X)) { throw Afina::POSIXException((#X)); }
#define VALIDATE_SYSTEM_FUNCTION(X) VALIDATE_CONDITION(((X) >= 0))

namespace Afina
{

struct POSIXException : public std::runtime_error
{
	POSIXException(const std::string& msg) : std::runtime_error(msg + ". Error description:" + std::strerror(errno))
	{}
};

struct NetworkException : public POSIXException
{
	NetworkException(const std::string& msg) : POSIXException("Network error: ")
	{}
};

} //namespace Afina

#endif // AFINA_DEBUG_H
