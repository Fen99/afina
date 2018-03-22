#ifndef AFINA_NETWORK_SOCKET_H
#define AFINA_NETWORK_SOCKET_H

#include <exception>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "./../../core/Debug.h"

//Macroses for check values after system callings (if errno was set)
#define VALIDATE_NETWORK_CONDITION(X) if(!(X)) { throw Afina::NetworkException((#X)); }
#define VALIDATE_NETWORK_FUNCTION(X) VALIDATE_NETWORK_CONDITION(((X) >= 0))

namespace Afina {
namespace Network {

class Socket 
{
	protected:
		int _socket_id;
		bool _opened;
		bool _is_nonblocking;

	protected:
		// Becomes an owner of the socket
		Socket(int socket_id, bool is_opened);
		Socket();

	public:
		enum class SOCKET_OPERATION_STATE
		{
			OK,
			NO_DATA_ASYNC,
			ERROR
		};

	protected:
		SOCKET_OPERATION_STATE _InterpretateReturnValue(int value);

	public:
		~Socket();

		//No-copiable
		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;

		//Movable
		Socket(Socket&& other);
		Socket& operator=(Socket&& other);

		void Shutdown(int shutdown_type = SHUT_RDWR);
		void Close();

		void MakeNonblocking();
		void MakeBlocking();

		bool GetSocketState() const { return _opened;         }
		bool IsNonblocking()  const { return _is_nonblocking; }
		int  GetSocketID()    const { return _socket_id;      }
};

} //namespace Network
} //namespace Afina

#endif // AFINA_NETWORK_SOCKET_H
