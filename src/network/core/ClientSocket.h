#ifndef AFINA_NETWORK_CLIENT_SOCKET_H
#define AFINA_NETWORK_CLIENT_SOCKET_H

#include "Socket.h"

#include <string>

#include <sys/uio.h>

namespace Afina {
namespace Network {

class ServerSocket;

class ClientSocket : public Socket
{
	public:
		struct IOInformation
		{
			IO_OPERATION_STATE state;
			int result;
		};

	private:
		ClientSocket();
		ClientSocket(int socket); //_opened = true (from accept)
		//Only server socket can create client sockets
		friend class ServerSocket;

	public:
		static const unsigned int reading_portion = 1024;
		//Recieved information will be append to "out" string
		IOInformation Receive(std::string& out, int count = reading_portion, bool wait_all = false);
		IOInformation Send(const std::string& data);
		IOInformation Send(const iovec iov[], int count);
};


} //namespace Network
} //namespace Afina

#endif //AFINA_NETWORK_CLIENT_SOCKET_H
