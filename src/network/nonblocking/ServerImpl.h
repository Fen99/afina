#ifndef AFINA_NETWORK_NONBLOCKING_SERVER_H
#define AFINA_NETWORK_NONBLOCKING_SERVER_H

#include <deque>

#include <afina/network/Server.h>

#include "Worker.h"
#include "./../core/ServerSocket.h"

namespace Afina {
namespace Network {
namespace NonBlocking {

/**
 * # Network resource manager implementation
 * Epoll based server
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint16_t workers = 1) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

private:
    std::shared_ptr<ServerSocket> _server_socket;

    // Thread that is accepting new connections
    std::deque<Worker> _workers;
};

} // namespace NonBlocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_NONBLOCKING_SERVER_H
