#ifndef AFINA_DEBUG_H
#define AFINA_DEBUG_H

#include <iostream>
#include <cstring>

#include <pthread.h>

#define NETWORK_DEBUG(X) std::cout << "network debug: " << X << std::endl
#define NETWORK_PROCESS_MESSAGE(MESSAGE) std::cout << "Process PID = " << pthread_self() << ": " << MESSAGE
#define NETWORK_PROCESS_DEBUG(PID, MESSAGE) NETWORK_DEBUG("Process PID = " << PID << ": " << MESSAGE)
#define NETWORK_CURRENT_PROCESS_DEBUG(MESSAGE) NETWORK_PROCESS_DEBUG(std::this_thread::get_id(), MESSAGE)

#define THREADPOOL_DEBUG(X) std::cout << "ThreadPool debug: " << X << std::endl
#define THREADPOOL_CURRENT_PROCESS_DEBUG(MESSAGE) THREADPOOL_DEBUG("Threadpool process PID = " << std::this_thread::get_id() << ": " << MESSAGE)

namespace Afina
{

struct NetworkException : public std::runtime_error
{
	NetworkException(const std::string& msg) : std::runtime_error(std::string("Network error: ") + msg + ". Error description:" + std::strerror(errno))
	{}
};

} //namespace Afina

#endif // AFINA_DEBUG_H
