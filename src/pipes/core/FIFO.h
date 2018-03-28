#ifndef AFINA_FIFO_H
#define AFINA_FIFO_H

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>

#include "./../../core/FileDescriptor.h"
#include "./../../core/Debug.h"

namespace Afina {
namespace FIFONamespace {

class FIFO : public Core::FileDescriptor
{
	public:
		enum class FIFO_READING_STATE
		{
			OK,
			TIMEOUT,
			NO_DATA_ASYNC,
			ERROR
		};

		enum class FIFO_WRITING_STATE
		{
			OK,
			NO_READERS,
			FIFO_FULL,
			ERROR
		};

		struct FIFOWrittenInformation
		{
			FIFO_WRITING_STATE state;
			int count_written;
		};

	private:
		FIFO_READING_STATE _Read(std::string& out, int count = PIPE_BUF);

	public:
		FIFO();

		//Creates fifo for both direction (optimal for our task):
		//For fifo, that is readed  --> to prevent EOF signal when the last process closing it for writing
		//-- || --          written --> to prevent ENXIO error on opening
		void Create(const std::string& name, bool is_blocking);

		//Can be stopped with SIGUSR1 signal
		FIFO_READING_STATE Read(std::string& out, int timeout = -1, int count = PIPE_BUF);
		FIFOWrittenInformation Write(const iovec iov[], int count);
};

} //namespace FIFONamespace
} //namespace Afina

#endif // _AFINA_FIFO_H
