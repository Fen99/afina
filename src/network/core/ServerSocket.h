#ifndef AFINA_NETWORK_SERVER_SOCKET_H
#define AFINA_NETWORK_SERVER_SOCKET_H

#include "Socket.h"
#include "ClientSocket.h"

namespace Afina {
namespace Network {

struct AcceptInformation; //In ClientSocket.h

class ServerSocket : public Socket
{
	public:
		struct AcceptInformation
		{
			IO_OPERATION_STATE state;
			ClientSocket socket;

			AcceptInformation(IO_OPERATION_STATE state, ClientSocket&& client_socket) : state(state), socket(std::move(client_socket))
			{}
		};

	public:
		ServerSocket();

		//If multiple_listeners = true, SO_REUSEPORT option will be set
		void Start(unsigned int port, unsigned int max_listeners, bool multiple_listeners = false);
		
		//If client_addr != nullptr, information about client will be writed to structure
		AcceptInformation Accept(sockaddr_in* client_addr = nullptr);
};

} //namespace Network
} //namespace Afina

#endif //AFINA_NETWORK_SERVER_SOCKET_H
