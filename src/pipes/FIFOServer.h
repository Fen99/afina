#ifndef AFINA_FIFO_SERVER_H
#define AFINA_FIFO_SERVER_H

#include <memory>
#include <unordered_map>
#include <thread>
#include <string>
#include <exception>
#include <atomic>

#include <afina/Storage.h>
#include "core/FIFO.h"
#include "./../protocol/Executor.h"

namespace Afina {
namespace FIFONamespace {

class FIFOServer
{
	private:
		std::shared_ptr<Storage> _storage;
		std::atomic<bool> _is_running;
		std::atomic<bool> _is_stopping;

		FIFO _reading_fifo;
		FIFO _writing_fifo;
		bool _has_out;

		std::thread _reading_thread;
		Protocol::Executor _executor;

		static const int _reading_timeout = 5;

		void _ThreadWrapper();
		void _ThreadFunction();

	public:
		FIFOServer(std::shared_ptr<Afina::Storage> storage);
		~FIFOServer();

		void Start(const std::string& reading_name, const std::string& writing_name);
		void Stop();
		void Join();
};

} //namespace Afina
} //namespace FIFONamespace

#endif // AFINA_FIFO_SERVER_H
