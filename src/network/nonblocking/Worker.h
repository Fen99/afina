#ifndef AFINA_NETWORK_NONBLOCKING_WORKER_H
#define AFINA_NETWORK_NONBLOCKING_WORKER_H

#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <exception>
#include <utility>
#include <functional>

#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <errno.h>

#include "./../../core/Debug.h"
#include "./../../protocol/Executor.h"
#include "./../core/ServerSocket.h"
#include "./../core/ClientSocket.h"

namespace Afina {

// Forward declaration, see afina/Storage.h
class Storage;

namespace Network {
namespace NonBlocking {

/**
 * # Thread running epoll
 * On Start spaws background thread that is doing epoll on the given server
 * socket and process incoming connections and its data
 */
class Worker {
public:
    Worker(std::shared_ptr<Afina::Storage> ps);
    ~Worker();

    /**
     * Spaws new background thread that is doing epoll on the given server
     * socket. Once connection accepted it must be registered and being processed
     * on this thread
     */
    void Start(std::shared_ptr<ServerSocket> server_socket, size_t max_listeners);

    /**
     * Signal background thread to stop. After that signal thread must stop to
     * accept new connections and must stop read new commands from existing. Once
     * all readed commands are executed and results are send back to client, thread
     * must stop
     */
    void Stop();

    /**
     * Blocks calling thread until background one for this worker is actually
     * been destoryed
     */
    void Join();

    int GetThreadId() { return _thread.native_handle(); }

private:
	enum class STATE {
		STOPPED,
		STOPPING,
		WORKS
	};

	struct ClientAndExecutor {
		ClientSocket client;
		Protocol::Executor executor;

		ClientAndExecutor(ClientSocket&& client_socket, std::shared_ptr<Afina::Storage> storage) : client(std::move(client_socket)), executor(storage)									{}
	};

private:
        /**
        * Method executing by background thread
	*/
    	void _ThreadWrapper(); //For exeptions
	void _ThreadFunction();

	static void _SignalHandler(int signal);

	bool _ReadFromSocket(int epoll, ClientAndExecutor& client_executor);
	bool _WriteToSocket(int epoll, ClientAndExecutor& client_executor);

private:
	std::thread _thread;
	std::atomic<STATE> _current_state; //independend on server state, because has Stop() function. atomic - can be changed out from _thread

	std::shared_ptr<ServerSocket> _server_socket;
	std::unordered_map<int, ClientAndExecutor> _clients;
	size_t _max_listeners;

	std::shared_ptr<Afina::Storage> _storage;
};

} // namespace NonBlocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_NONBLOCKING_WORKER_H
