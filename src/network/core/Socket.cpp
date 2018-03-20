#include "Socket.h"

namespace Afina {
namespace Network {

Socket::Socket(int socket, bool is_opened) : _socket_id(socket), _opened(is_opened)
{}

Socket::~Socket() {
	Close();
}

Socket::Socket() : _socket_id(-1), _opened(false)
{}

void Socket::Shutdown(int shutdown_type) {
	VALIDATE_NETWORK_FUNCTION(shutdown(_socket_id, shutdown_type));
}

void Socket::Close() {
	if (_opened) { VALIDATE_NETWORK_FUNCTION(close(_socket_id)); }
}

}
}