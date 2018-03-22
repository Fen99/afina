#include "ServerImpl.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <afina/Storage.h>

namespace Afina {
namespace Network {
namespace NonBlocking {

// See Server.h
ServerImpl::ServerImpl(std::shared_ptr<Afina::Storage> ps) : Server(ps), _server_socket(std::make_shared<ServerSocket>()) {}

// See Server.h
ServerImpl::~ServerImpl() {
	Stop();
}

// See Server.h
void ServerImpl::Start(uint16_t port, uint16_t n_workers) {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);

    // If a client closes a connection, this will generally produce a SIGPIPE
    // signal that will kill the process. We want to ignore this signal, so send()
    // just returns -1 when this happens.
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &sig_mask, NULL) != 0) {
        throw std::runtime_error("Unable to mask SIGPIPE");
    }

    // Create server socket
    _server_socket->Start(port, max_listen, true);
    _server_socket->MakeNonblocking();
    
    for (int i = 0; i < n_workers; i++) {
	_workers.emplace_back(pStorage);
    }
    for (auto it = _workers.begin(); it != _workers.end(); it++) {
    	it->Start(_server_socket, max_listen);
    }
}

// See Server.h
void ServerImpl::Stop() {
    NETWORK_DEBUG(__PRETTY_FUNCTION__);
    for (auto it = _workers.begin(); it != _workers.end(); it++) {
        it->Stop();
    }
}

// See Server.h
void ServerImpl::Join() {
    NETWORK_DEBUG(__PRETTY_FUNCTION__);
     for (auto it = _workers.begin(); it != _workers.end(); it++) {
        it->Join();
    }
}

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
