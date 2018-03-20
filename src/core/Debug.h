#ifndef AFINA_DEBUG_H
#define AFINA_DEBUG_H

#include <iostream>
#include <pthread.h>

#define NETWORK_DEBUG(X) std::cout << "network debug: " << X << std::endl
#define NETWORK_PROCESS_DEBUG(PID, MESSAGE) NETWORK_DEBUG("Process PID = " << PID << ": " << MESSAGE)
#define NETWORK_PROCESS_MESSAGE(MESSAGE) std::cout << "Process PID = " << pthread_self() << ": " << MESSAGE

#define THREADPOOL_DEBUG(X) std::cout << "ThreadPool debug: " << X << std::endl
#define THREADPOOL_PROCESS_DEBUG(MESSAGE) THREADPOOL_DEBUG("Threadpool process PID = " << std::this_thread::get_id() << ": " << MESSAGE)

#endif // AFINA_DEBUG_H
