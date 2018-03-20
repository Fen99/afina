#ifndef AFINA_NETWORK_SOCKET_H
#define AFINA_NETWORK_SOCKET_H

#include <exception>
#include <cerrno>

#include <sys/socket.h>

#include "./../core/Debug.h"

#define VALIDATE_NETWORK_FUNCTION(X) if(X < 0) {throw std::runtime_error(std::string("Network error: ")+std::strerror(errno));}

namespace Afina {
namespace Network {

class Socket 
{
	protected:
		int _socket_id;
		bool _opened;

	protected:
		// Becomes an owner of the socket
		Socket(int socket_id, bool is_opened);
		Socket();

	public:
		~Socket();

		//Not-copiable
		Socket(const Socket& socket) = delete;
		Socket& operator=(const Socket& socket) = delete;

		void Shutdown(int shutdown_type);
		void Close();
};

}
}

#endif // AFINA_NETWORK_SOCKET_H
