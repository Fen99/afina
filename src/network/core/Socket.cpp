#include "Socket.h"

namespace Afina {
namespace Network {

Socket::Socket(int socket, bool is_opened) : _socket_id(socket), _opened(is_opened), _is_nonblocking(false)
{}

Socket::Socket() : _socket_id(-1), _opened(false), _is_nonblocking(false)
{}

Socket::Socket(Socket&& other) : _socket_id(other._socket_id), _opened(other._opened), _is_nonblocking(other._is_nonblocking)
{
    other._opened = false;
}

Socket& Socket::operator=(Socket&& other) 
{
    Close();

    _socket_id = other._socket_id;
    _opened = other._opened;
    _is_nonblocking = other._is_nonblocking;

    other._opened = false;
}


Socket::~Socket() {
	Close();
}

Socket::SOCKET_OPERATION_STATE Socket::_InterpretateReturnValue(int value)
{
	if (value < 0)
	{
		if (value == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) { return SOCKET_OPERATION_STATE::NO_DATA_ASYNC; }
		else { return SOCKET_OPERATION_STATE::ERROR; }
	}

	return SOCKET_OPERATION_STATE::OK;
}

void Socket::Shutdown(int shutdown_type) {
	VALIDATE_NETWORK_FUNCTION(shutdown(_socket_id, shutdown_type));
}

void Socket::Close() {
	if (_opened)
	{ 
		VALIDATE_NETWORK_FUNCTION(close(_socket_id));
		NETWORK_DEBUG("Socket " << _socket_id << " was closed");
	}
	_opened = false;
}

void Socket::MakeNonblocking()
{
	if (_is_nonblocking) { return; }

	int flags = fcntl(_socket_id, F_GETFL, 0);
	VALIDATE_NETWORK_FUNCTION(flags = fcntl(_socket_id, F_GETFL, 0));

	flags |= O_NONBLOCK;
	VALIDATE_NETWORK_FUNCTION(fcntl(_socket_id, F_SETFL, flags));

	_is_nonblocking = true;
}

void Socket::MakeBlocking()
{
	if (!_is_nonblocking) { return; }

	int flags = fcntl(_socket_id, F_GETFL, 0);
	VALIDATE_NETWORK_FUNCTION(flags = fcntl(_socket_id, F_GETFL, 0));

	flags &= ~O_NONBLOCK;
	VALIDATE_NETWORK_FUNCTION(fcntl(_socket_id, F_SETFL, flags));
}

}
}
