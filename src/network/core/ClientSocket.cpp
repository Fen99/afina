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

	IOInformation info = {IO_OPERATION_STATE::OK, 0};
	while (count > 0)
	{
		char new_data [reading_portion] = "";
		int result = recv(_fd_id, new_data, reading_portion * sizeof(char), (wait_all ? MSG_WAITALL : 0));

		info.state = _InterpretateReturnValue(result);
		if (result > 0)
		{
			out.append(new_data);
			info.result += result;
			count -= reading_portion;
		}
		else
		{
			break;
		}
	}

	return info;
}

ClientSocket::IOInformation ClientSocket::Send(const std::string& data)
{
	if (!_opened) { throw NetworkException("Cannot use ClientSocket from ServerSocket::AcceptInformation structure with incorrect state!"); }

	//(int) ((size_t) -1) = -1
	int result = send(_fd_id, data.c_str(), data.size(), 0);
	IOInformation info = {IO_OPERATION_STATE::OK, result};
	info.state = _InterpretateReturnValue(result);
	return info;
}

ClientSocket::IOInformation ClientSocket::Send(const iovec iov[], int count)
{
	if (!_opened) { throw NetworkException("Cannot use ClientSocket from ServerSocket::AcceptInformation structure with incorrect state!"); }

	int result = writev(_fd_id, iov, count);
	IOInformation info = {IO_OPERATION_STATE::OK, result};
	info.state = _InterpretateReturnValue(result);
	return info;
}

} //namespace Network
} //namespace Afina
