#include <afina/execute/Command.h>

bool Command::ExtractArguments(std::string& args_str) {
	if (args_str.size() > 8) { //" noreply"
		if (args_str.substr(args_str.size() - 8) == " noreply") {
			_no_reply = true;
			args_str = args_str.substr(0, args_str_work.size() - 8);
		}
	}
}