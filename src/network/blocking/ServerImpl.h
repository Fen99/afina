#ifndef AFINA_NETWORK_BLOCKING_SERVER_H
#define AFINA_NETWORK_BLOCKING_SERVER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pthread.h>
#include <unordered_set>

#include <afina/network/Server.h>
#include "./../../protocol/Parser.h"
#include <afina/execute/Command.h>
#include "./../../core/ThreadPool.h"

namespace Afina {
namespace Network {
namespace Blocking {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint16_t workers) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

    ServerImpl(const ServerImpl&) = delete;
    ServerImpl& operator=(const ServerImpl&) = delete;

protected:
    /**
     * Method is running in the connection acceptor thread
	 * Int parameter is ignored (for compatibly with RunMethodInDifferentThread function)
     */
    void RunAcceptor(int socket = 0);

    /**
     * Methos is running for each connection
     */
	void RunConnection(int client_socket = 0);

private:
    //Type for transporting client socket num to funcions
    struct ThreadParams {
	ServerImpl* server;
	int parameter;

	ThreadParams(ServerImpl* srv, int param) : server(srv), parameter(param) {}
    };

    //Function for pthread_create. pthread_create gets this pointer as parameter
    //and then this function calls RunAcceptor()/RunConnectionProxy
    //Template argument - pointer to function-member, that should be started
    //void* parameter should be ThreadParams structure, created with new (delete will be called by this function) 
    template <void (ServerImpl::*function_for_start)(int)>
    static void *RunMethodInDifferentThread(void *p)
	{
		ThreadParams *params = reinterpret_cast<ThreadParams *>(p);
		try
		{ //! For exceptions in threads !
			(params->server->*function_for_start)(params->parameter);
		}
		catch (std::runtime_error &ex)
		{
			std::cerr << "Server fails: " << ex.what() << std::endl;
		}
		delete params;
		return 0;
	}


    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publish changes cross thread
    // bounds
    std::atomic<bool> running;
    //atomic to differentiate accept() fails
    std::atomic<bool> _is_finishing;

    // Thread that is accepting new connections
    pthread_t accept_thread;

	// Server socket
	int _server_socket;

    // Port to listen for new connections, permits access only from
    // inside of accept_thread
    // Read-only
    uint16_t listen_port;

    // Mutex used to access connections list
    std::mutex connections_mutex;

    // Conditional variable used to notify waiters about empty
    // connections list
    std::condition_variable connections_cv;

    // Treadpool for new threads
	Core::ThreadPool _thread_pool;
	// Client sockets
	std::unordered_set<int> _client_sockets;

	// Maximal count of clients to socket in listen() function
	static const int _max_listen = 5;
};

} // namespace Blocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_BLOCKING_SERVER_H
