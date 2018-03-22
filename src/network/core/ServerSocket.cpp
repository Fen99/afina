#include <utility>

#include "ServerSocket.h"

namespace Afina {
namespace Network {

ServerSocket::ServerSocket() : Socket()
{}

void ServerSocket::Start(unsigned int port, unsigned int max_listeners, bool multiple_listeners)
{
	// For IPv4 we use struct sockaddr_in:
	// struct sockaddr_in {
	//     short int          sin_family;  // Address family, AF_INET
	//     unsigned short int sin_port;    // Port number
	//     struct in_addr     sin_addr;    // Internet address
	//     unsigned char      sin_zero[8]; // Same size as struct sockaddr
	// };
	//
	// Note we need to convert the port to network order
	sockaddr_in server_addr = {};
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;          // IPv4
	server_addr.sin_port = htons(port); // TCP port number
	server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any address

	// Arguments are:
    // - Family: IPv4
    // - Type: Full-duplex stream (reliable)
    // - Protocol: TCP
	VALIDATE_NETWORK_FUNCTION(_socket_id = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
	_opened = true;
	NETWORK_DEBUG("Server socket " << _socket_id << " was created");

	// when the server closes the socket,the connection must stay in the TIME_WAIT state to
	// make sure the client received the acknowledgement that the connection has been terminated.
	// During this time, this port is unavailable to other processes, unless we specify this option
	//
	// This option let kernel knows that we are OK that multiple threads/processes are listen on the
	// same port. In a such case kernel will balance input traffic between all listeners (except those who
	// are closed already)
	int opts = 1;
	VALIDATE_NETWORK_FUNCTION(setsockopt(_socket_id, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts)));

	//For non-blocking multiple threads
	if (multiple_listeners)
	{
		VALIDATE_NETWORK_FUNCTION(setsockopt(_socket_id, SOL_SOCKET, SO_REUSEPORT, &opts, sizeof(opts)));
	}

	// Bind the socket to the address. In other words let kernel know data for what address we'd
	// like to see in the socket
	VALIDATE_NETWORK_FUNCTION(bind(_socket_id, (sockaddr*) &server_addr, sizeof(server_addr)));

	// Start listening. The second parameter is the "backlog", or the maximum number of
	// connections that we'll allow to queue up. Note that listen() (!)doesn't block until
	// incoming connections arrive. It just makes the OS aware that this process is willing
	// to accept connections on this socket (which is bound to a specific IP and port)
	VALIDATE_NETWORK_FUNCTION(listen(_socket_id, max_listeners));
}

ServerSocket::AcceptInformation ServerSocket::Accept(sockaddr_in* client_addr)
{
	if (client_addr == nullptr)
	{
		sockaddr_in client_addr = {};
		std::memset(&client_addr, 0, sizeof(client_addr));
		return Accept(&client_addr);
	}

	socklen_t sinSize = sizeof(sockaddr_in);
	int result = accept(_socket_id, (sockaddr*) &client_addr, &sinSize);
	
	SOCKET_OPERATION_STATE state = _InterpretateReturnValue(result);
	if (state == SOCKET_OPERATION_STATE::OK)
	{
		NETWORK_DEBUG("Client socket " << result << " was created");
		ClientSocket client(result);
		return AcceptInformation(SOCKET_OPERATION_STATE::OK, std::move(client));
	}
	else
	{
		ClientSocket client; //default constructor - incorrect socket
		return AcceptInformation(state, std::move(client));
	}
}

} //namespace Network
} //namespace Afina
