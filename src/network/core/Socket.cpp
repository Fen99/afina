#include "Socket.h"

namespace Afina {
namespace Network {

Socket::Socket(int socket, bool is_opened) : Core::FileDescriptor(socket, is_opened)
{}

Socket::Socket() : Core::FileDescriptor()
{}

void Socket::Shutdown(int shutdown_type) {
	VALIDATE_NETWORK_FUNCTION(shutdown(_fd_id, shutdown_type));
}

}
}
