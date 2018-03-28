#ifndef AFINA_FILE_DESCRIPTOR_H
#define AFINA_FILE_DESCRIPTOR_H

#include <exception>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Debug.h"

namespace Afina {
namespace Core {

class FileDescriptor 
{
	protected:
		int _fd_id;
		bool _opened;
		bool _is_nonblocking;

	protected:
		// Becomes an owner of the file descriptor
		FileDescriptor(int fd_id, bool is_opened);
		FileDescriptor();

	public:
		enum class IO_OPERATION_STATE
		{
			OK,
			ASYNC_ERROR,
			ERROR
		};

	protected:
		IO_OPERATION_STATE _InterpretateReturnValue(int value);

	public:
		~FileDescriptor();

		//No-copiable
		FileDescriptor(const FileDescriptor&) = delete;
		FileDescriptor& operator=(const FileDescriptor&) = delete;

		//Movable
		FileDescriptor(FileDescriptor&& other);
		FileDescriptor& operator=(FileDescriptor&& other);

		void Close();

		void MakeNonblocking();
		void MakeBlocking();

		bool GetSocketState() const { return _opened;         }
		bool IsNonblocking()  const { return _is_nonblocking; }
		int  GetID() const { return _fd_id; }
};

} //namespace Core
} //namespace Afina

#endif // AFINA_FILE_DESCRIPTOR_H
