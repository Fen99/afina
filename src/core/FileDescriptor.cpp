#include "FileDescriptor.h"

namespace Afina {
namespace Core {

FileDescriptor::FileDescriptor(int fd, bool is_opened) : _fd_id(fd), _opened(is_opened), _is_nonblocking(false)
{}

FileDescriptor::FileDescriptor() : _fd_id(-1), _opened(false), _is_nonblocking(false)
{}

FileDescriptor::FileDescriptor(FileDescriptor&& other) : _fd_id(other._fd_id), _opened(other._opened),
														 _is_nonblocking(other._is_nonblocking)
{
    other._opened = false;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other)
{
    Close();

    _fd_id = other._fd_id;
    _opened = other._opened;
    _is_nonblocking = other._is_nonblocking;

    other._opened = false;
}


FileDescriptor::~FileDescriptor() {
	Close();
}

FileDescriptor::IO_OPERATION_STATE FileDescriptor::_InterpretateReturnValue(int value)
{
	if (value < 0)
	{
		if (value == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) { return IO_OPERATION_STATE::ASYNC_ERROR; }
		else { return IO_OPERATION_STATE::ERROR; }
	}

	return IO_OPERATION_STATE::OK;
}

void FileDescriptor::Close() {
	if (_opened)
	{ 
		VALIDATE_SYSTEM_FUNCTION(close(_fd_id));
		CURRENT_PROCESS_DEBUG("File descriptor " << _fd_id << " was closed");
	}
	_opened = false;
}

void FileDescriptor::MakeNonblocking()
{
	if (_is_nonblocking) { return; }

	int flags = fcntl(_fd_id, F_GETFL, 0);
	VALIDATE_SYSTEM_FUNCTION(flags = fcntl(_fd_id, F_GETFL, 0));

	flags |= O_NONBLOCK;
	VALIDATE_SYSTEM_FUNCTION(fcntl(_fd_id, F_SETFL, flags));

	_is_nonblocking = true;
}

void FileDescriptor::MakeBlocking()
{
	if (!_is_nonblocking) { return; }

	int flags = fcntl(_fd_id, F_GETFL, 0);
	VALIDATE_SYSTEM_FUNCTION(flags = fcntl(_fd_id, F_GETFL, 0));

	flags &= ~O_NONBLOCK;
	VALIDATE_SYSTEM_FUNCTION(fcntl(_fd_id, F_SETFL, flags));
}

}
}
