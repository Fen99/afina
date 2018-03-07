#ifndef AFINA_NETWORK_BLOCKING_SERVER_H
#define AFINA_NETWORK_BLOCKING_SERVER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pthread.h>
#include <unordered_set>

#include <afina/network/Server.h>
#include <afina/protocol/Parser.h>

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
	//Function for pthread_create. pthread_create gets this pointer as parameter
	//and then this function calls RunAcceptor()/RunConnectionProxy
	//Template argument - pointer to function-member, that should be started
	//Additional work: 
	template <ServerImpl::*function_for_start(int)>
    static void *RunMethodInDifferentThread(void *p);

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publish changes cross thread
    // bounds
    std::atomic<bool> running;

    // Thread that is accepting new connections
    pthread_t accept_thread;

	// Server socket
	int _server_socket;

    // Maximum number of client allowed to exists concurrently
    // on server, permits access only from inside of accept_thread.
    // Read-only
    uint16_t max_workers;

    // Port to listen for new connections, permits access only from
    // inside of accept_thread
    // Read-only
	uint16_t listen_port;

    // Mutex used to access connections list
    std::mutex connections_mutex;

    // Conditional variable used to notify waiters about empty
    // connections list
    std::condition_variable connections_cv;

    // Threads that are processing connection data, permits
    // access only from inside of accept_thread and workers threads (?)
    std::unordered_set<pthread_t> connections;
	// Client sockets
	std::unordered_set<int> _client_sockets;

	// Maximal count of clients to socket in listen() function
	static const int _max_listen = 5;
};

} // namespace Blocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_BLOCKING_SERVER_H
