#include "Executor.h"

//===============================================================================================

namespace Afina {
namespace Protocol {

Executor::Command::Command() : _command_object(), _arg_size(0) 
{}

void Executor::Command::Reset()
{
	_command_object.release();
	_arg_size = 0;
}

void Executor::Command::SetNewCommand(command_ptr&& command_object, size_t arg_size)
{
	_command_object = std::move(command_object);
	if (arg_size == 0) { _arg_size = 0; }
	else { _arg_size = arg_size + 2; } //\r\n
}

//===============================================================================================

Executor::Executor(std::shared_ptr<Afina::Storage> storage) : _storage(storage)
{}

void Executor::_AddLineToQueue(const std::string& msg)
{
	_output_queue.push_back(msg+"\r\n");

	iovec new_iov = {(void*) _output_queue.back().c_str(), _output_queue.back().size()};
	_iovec_output.push_back(std::move(new_iov));
}

void Executor::_Reset(bool clear_data)
{
	_parser.Reset();
	_current_command.Reset();

	if (clear_data) { _current_string = ""; }
}

void Executor::_Execute()
{
	std::string argument;
	std::string out;
	if (_current_command.ArgumentSize() != 0) //Command need argument
	{
		argument = _current_string.substr(0, _current_command.ArgumentSize());
		_current_string = _current_string.substr(_current_command.ArgumentSize()); //remove argument from received data

		if (argument.substr(argument.size() - 2) != "\r\n")
		{
			//_AddLineToQueue(std::string("Parsing error: ") + "command argument should finish by \\r\\n");
			_AddLineToQueue("ERROR");
			_Reset(false);
			return;
		}
		argument = argument.substr(0, argument.size() - 2);  // \r\n not needed
	}

	try { _current_command.CommandObject()->Execute(*_storage, argument, out); }
	catch (std::exception& e) {
		out = "ERROR";
		//out = "SERVER ERROR ";
		//out += e.what();
	}

	_AddLineToQueue(out);
	_Reset(false);
}

bool Executor::_ReadOneCommand()
{
	bool was_command = false;
	size_t parsed = 0;
	try { was_command = _parser.Parse(_current_string, parsed); }
	catch (std::exception& e)
	{
		//_AddLineToQueue(std::string("Parsing error: ") + e.what());
		_AddLineToQueue("ERROR");
		_Reset(true);
		return true;
	}

	_current_string = _current_string.substr(parsed); //remove parsed part of string (was saved in parser) <or> remove command
	if (!was_command) { return false; } //need more data

	uint32_t arg_size = 0;
	auto command_object = _parser.Build(arg_size);
	_current_command.SetNewCommand(std::move(command_object), arg_size);

	if (_current_command.ArgumentSize() > _current_string.size()) { return false; } //need more data
	else
	{
		_Execute(); //Calls _Reset
		return true;
	}
}

bool Executor::AppendAndTryExecute(const std::string& str)
{
	_current_string.append(str);
	bool command_result = false;
	while (_ReadOneCommand())
	{
	    command_result = true;
	}
	return command_result;
}

std::string Executor::GetWholeOutputAsString(bool remove)
{
	std::string result;
	for (auto it = _output_queue.cbegin(); it != _output_queue.cend(); it++)
	{
		result += (*it);
	}

	if (remove)
	{
		_output_queue.clear();
		_iovec_output.clear();
	}

	return result;
}

const iovec* Executor::GetOutputAsIovec() const
{
	return _iovec_output.data();
}

void Executor::RemoveFromOutput(unsigned int bytes)
{
	size_t count_full_buffers = 0;
	for (auto it = _output_queue.cbegin(); it != _output_queue.cend(); it++)
	{
		if (it->size() <= bytes)
		{ 
			++count_full_buffers;
			bytes -= it->size();
		}
		else { break; }
	}

	//Erase full buffers
	_output_queue.erase(_output_queue.begin(), _output_queue.begin() + count_full_buffers);
	_iovec_output.erase(_iovec_output.begin(), _iovec_output.begin() + count_full_buffers);

	//Need shrink the last buffer
	if (!_output_queue.empty() && bytes != 0)
	{
		_output_queue[0] = _output_queue[0].substr(bytes);
		_iovec_output[0].iov_base = (void*) _output_queue[0].c_str();
		_iovec_output[0].iov_len  =  _output_queue[0].size();
	}
}

} // namespace Protocol
} // namespace Afina
