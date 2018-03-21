#ifndef AFINA_NETWORK_NONBLOCKING_SERVER_H
#define AFINA_NETWORK_NONBLOCKING_SERVER_H

#include <vector>

#include <afina/network/Server.h>

#include "./../core/ServerSocket.h"

namespace Afina {
namespace Network {
namespace NonBlocking {

// Forward declaration, see Worker.h
class Worker;

/**
 * # Network resource manager implementation
 * Epoll based server
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

private:
	std::shared_ptr<ServerSocket> _server_socket;

    // Thread that is accepting new connections
    std::vector<Worker> _workers;
};

} // namespace NonBlocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_NONBLOCKING_SERVER_H
