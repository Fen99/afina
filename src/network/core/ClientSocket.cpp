#include <errno.h>

#include "ClientSocket.h"

namespace Afina {
namespace Network {

ClientSocket::ClientSocket(int socket) : Socket(socket, true)
{}

ClientSocket::ClientSocket() : Socket()
{}

ClientSocket::IOInformation ClientSocket::Receive(std::string& out, int count, bool wait_all)
{
	if (!_opened) { throw NetworkException("Cannot use ClientSocket from ServerSocket::AcceptInformation structure with incorrect state!"); }

	char new_data [reading_portion] = "";
	int result = recv(_socket_id, new_data, count * sizeof(char), (wait_all ? MSG_WAITALL : 0));

	IOInformation info = {SOCKET_OPERATION_STATE::OK, result};
	info.state = _InterpretateReturnValue(result);
	if (result > 0)
	{
		out.append(new_data);
	}

	return info;
}

ClientSocket::IOInformation ClientSocket::Send(const std::string& data)
{
	if (!_opened) { throw NetworkException("Cannot use ClientSocket from ServerSocket::AcceptInformation structure with incorrect state!"); }

	//(int) ((size_t) -1) = -1
	int result = send(_socket_id, data.c_str(), data.size(), 0);
	IOInformation info = {SOCKET_OPERATION_STATE::OK, result};
	info.state = _InterpretateReturnValue(result);
	return info;
}

ClientSocket::IOInformation ClientSocket::Send(const iovec iov[], int count)
{
	if (!_opened) { throw NetworkException("Cannot use ClientSocket from ServerSocket::AcceptInformation structure with incorrect state!"); }

	int result = writev(_socket_id, iov, count);
	IOInformation info = {SOCKET_OPERATION_STATE::OK, result};
	info.state = _InterpretateReturnValue(result);
	return info;
}

} //namespace Network
} //namespace Afina
