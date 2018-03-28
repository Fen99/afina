#ifndef AFINA_NETWORK_SOCKET_H
#define AFINA_NETWORK_SOCKET_H

#include <exception>
#include <cerrno>

#include <sys/socket.h>

#include "./../../core/FileDescriptor.h"

#define FORMAT_NETWORK_MESSAGE(MESSAGE) "Network debug: " << MESSAGE
#define NETWORK_DEBUG(MESSAGE) std::cout << FORMAT_NETWORK_MESSAGE(MESSAGE) << std::endl
#define NETWORK_PROCESS_DEBUG(PID, MESSAGE) PROCESS_DEBUG(PID, FORMAT_NETWORK_MESSAGE(MESSAGE))
#define NETWORK_CURRENT_PROCESS_DEBUG(MESSAGE) CURRENT_PROCESS_DEBUG(FORMAT_NETWORK_MESSAGE(MESSAGE))

//Macroses for check values after system callings (if errno was set)
#define VALIDATE_NETWORK_CONDITION(X) if(!(X)) { throw Afina::NetworkException((#X)); }
#define VALIDATE_NETWORK_FUNCTION(X) VALIDATE_NETWORK_CONDITION(((X) >= 0))

namespace Afina {
namespace Network {

class Socket : public Core::FileDescriptor
{
	protected:
		// Becomes an owner of the socket
		Socket(int socket_id, bool is_opened);
		Socket();

	public:
		void Shutdown(int shutdown_type = SHUT_RDWR);
};

} //namespace Network
} //namespace Afina

#endif // AFINA_NETWORK_SOCKET_H