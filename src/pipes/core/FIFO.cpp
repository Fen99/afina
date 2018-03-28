#include "FIFO.h"

namespace Afina {
namespace FIFONamespace {

FIFO::FIFO() : FileDescriptor()
{}

void FIFO::Create(const std::string& name, bool is_blocking)
{
	remove(name.c_str());
	VALIDATE_SYSTEM_FUNCTION(mkfifo(name.c_str(), S_IWUSR | S_IRUSR));
	VALIDATE_SYSTEM_FUNCTION(_fd_id = open(name.c_str(), O_RDWR));
	_opened = true;

	CURRENT_PROCESS_DEBUG("FIFO " << name << "; id = " << _fd_id << " was opened");

	if (is_blocking) { MakeBlocking(); }

	struct sigaction usr_signal;
	usr_signal.sa_handler = [](int){};
	VALIDATE_SYSTEM_FUNCTION(sigaction(SIGUSR1, &usr_signal, NULL));
}

FIFO::FIFO_READING_STATE FIFO::_Read(std::string& out, int count)
{
	IO_OPERATION_STATE info = IO_OPERATION_STATE::OK;
	while (count > 0)
	{
		char new_data[PIPE_BUF] = "";
		size_t result = read(_fd_id, new_data, PIPE_BUF);

		info = _InterpretateReturnValue(result);
		if (result > 0)
		{
			out.append(new_data);
			count -= PIPE_BUF;
		}
		else { break; }
	}

	switch (info)
	{
		case IO_OPERATION_STATE::OK:
			return FIFO_READING_STATE::OK;
		case IO_OPERATION_STATE::ASYNC_ERROR:
			return FIFO_READING_STATE::NO_DATA_ASYNC;
		case IO_OPERATION_STATE::ERROR:
			return FIFO_READING_STATE::ERROR;
	}
}

FIFO::FIFO_READING_STATE FIFO::Read(std::string& out, int count, int timeout)
{
	if (!_opened) { throw POSIXException("Cannot read from closed fifo!"); }

	if (_is_nonblocking || timeout == -1) { return _Read(out, count); }

	pollfd descriptor;
	descriptor.fd = _fd_id;
	descriptor.events = POLLIN | POLLRDHUP;

	int result = poll(&descriptor, 1, timeout);
	if (result == -1) { return FIFO_READING_STATE::ERROR; }
	if (result == 0)  { return FIFO_READING_STATE::TIMEOUT; }

	if (descriptor.revents & POLLIN) { return _Read(out, count); }
	else { return FIFO_READING_STATE::ERROR; } //any error occured
}

FIFO::FIFOWrittenInformation FIFO::Write(const iovec iov[], int count)
{
	if (!_opened) { throw NetworkException("Cannot write to closed fifo"); }

	int result = writev(_fd_id, iov, count);
	FIFO::FIFOWrittenInformation info = {FIFO_WRITING_STATE::OK, result};
	if (result < 0)
	{
		switch (errno)
		{
			case EAGAIN:
				info.state = FIFO_WRITING_STATE::FIFO_FULL;
				break;
			case EPIPE:
				info.state = FIFO_WRITING_STATE::NO_READERS;
				break;
			default:
				info.state = FIFO_WRITING_STATE::ERROR;
		}
	}

	return info;
}

}
}
