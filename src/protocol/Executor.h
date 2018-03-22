#ifndef AFINA_EXECUTOR_H
#define AFINA_EXECUTOR_H

#include <string>
#include <deque>
#include <vector>
#include <utility>
#include <memory>

#include <sys/uio.h>

#include <afina/Storage.h>
#include <afina/execute/Command.h>

#include "Parser.h"
#include "./../core/Debug.h"

namespace Afina {
class Storage;

namespace Protocol {

//Interpretates command string and forms output
class Executor
{
	private:
		class Command
		{
			public:
				typedef std::unique_ptr<Execute::Command> command_ptr;

			private:
				command_ptr _command_object;
				size_t _arg_size;

			public:
				Command();

				void Reset();
				void SetNewCommand(command_ptr&& command_object, size_t arg_size);

				const command_ptr& CommandObject() const { return _command_object; }
				size_t ArgumentSize() const { return _arg_size; }

		};

	private:
		std::shared_ptr<Afina::Storage> _storage;

		std::string _current_string;
		Parser _parser;
		Command _current_command;

		std::deque<std::string> _output_queue;
		std::vector<iovec> _iovec_output;

	private:
		void _AddLineToQueue(const std::string& msg);
		void _Reset(bool clear_data);
		
		bool _ReadOneCommand();

		// Executes _current_command. Assumes that _current_string is enough for command argument
		void _Execute();

	public:
		Executor(std::shared_ptr<Afina::Storage> storage);

		//Returns true if new data is avaliable
		bool AppendAndTryExecute(const std::string& str);
		
		std::string GetWholeOutputAsString(bool remove = false);
		const iovec* GetOutputAsIovec() const;
		size_t GetQueueSize() const { return _iovec_output.size(); }

		bool HasOutputData() const { return !_output_queue.empty(); }
		void RemoveFromOutput(unsigned int bytes);
};

} // namespace Protocol
} // namespace Afina

#endif // AFINA_EXECUTOR_H
