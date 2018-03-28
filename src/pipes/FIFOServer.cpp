#include "FIFOServer.h"

namespace Afina {
namespace FIFONamespace {

FIFOServer::FIFOServer(std::shared_ptr<Afina::Storage> storage) : _storage(storage), _reading_fifo(), _is_running(false),																  _is_stopping(false), _executor(storage), _has_out(false)
{}

FIFOServer::~FIFOServer()
{
	Stop();
}

void FIFOServer::Start(const std::string& reading_name, const std::string& writing_name)
{
	if (_is_running || _is_stopping) { return; }
	_reading_fifo.Create(reading_name, true);
	if (writing_name != "") {
		_writing_fifo.Create(writing_name, false);
		_has_out = true;
	}

	_is_running.store(true);
	_reading_thread = std::thread(&FIFOServer::_ThreadWrapper, this);
}

void FIFOServer::Stop()
{
	if (!_is_running.load() || _is_stopping.load()) { return; }

	_is_stopping.store(true);
	pthread_kill(_reading_thread.native_handle(), SIGUSR1);
	Join();

	_is_running.store(false);
	_is_stopping.store(false);
}

void FIFOServer::Join()
{
	if (_reading_thread.joinable()) { _reading_thread.join(); }
}

void FIFOServer::_ThreadWrapper()
{
	try
	{
		CURRENT_PROCESS_DEBUG(std::string("FIFO thread was started"));
		_ThreadFunction();
	}
	catch (std::exception exc)
	{
		CURRENT_PROCESS_DEBUG(std::string("FIFO thread was failed with an error: ") + exc.what());
	}
}

void FIFOServer::_ThreadFunction()
{
	while (_is_running.load())
	{
		std::string new_data;
		auto result = _reading_fifo.Read(new_data, _reading_timeout);
		if (result == FIFO::FIFO_READING_STATE::ERROR)
		{
			if (!_is_stopping.load()) { throw POSIXException("Unable to read from pipe!"); }
			else { break; }
		}

		if (result != FIFO::FIFO_READING_STATE::TIMEOUT) { _executor.AppendAndTryExecute(new_data); }

		if (!_has_out) { _executor.ClearOutput(); }
		if (_executor.HasOutputData())
		{
			auto result = _writing_fifo.Write(_executor.GetOutputAsIovec(), _executor.GetQueueSize());
			if (result.state == FIFO::FIFO_WRITING_STATE::ERROR) { throw POSIXException("Unable write to pipe!"); }
			if (result.state == FIFO::FIFO_WRITING_STATE::OK) { _executor.RemoveFromOutput(result.count_written); }
		}
	}

	//The last attemp to write to output fifo
	if (_executor.HasOutputData()) { _writing_fifo.Write(_executor.GetOutputAsIovec(), _executor.GetQueueSize()); }
}

}
}
