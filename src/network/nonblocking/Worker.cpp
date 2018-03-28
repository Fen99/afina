#include "Worker.h"

#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

namespace Afina {
namespace Network {
namespace NonBlocking {

// See Worker.h
Worker::Worker(std::shared_ptr<Afina::Storage> ps) : _storage(ps), _current_state(STATE::STOPPED), _max_listeners(0)
{}

// See Worker.h
Worker::~Worker() {
	Join();
}

// See Worker.h
void Worker::Start(std::shared_ptr<ServerSocket> server_socket, size_t max_listeners) {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);
    
	if (!server_socket->IsNonblocking()) {
		throw std::runtime_error("Worker can accept only non-blocking server sockets!");
	}

	_max_listeners = max_listeners;
	_server_socket = server_socket;

	//Register signal to stop epoll
	struct sigaction sa = {};
	sa.sa_handler = Worker::_SignalHandler;
	VALIDATE_NETWORK_FUNCTION(sigaction(SIGUSR1, &sa, NULL));

	_current_state.store(STATE::WORKS);
	_thread = std::thread(&Worker::_ThreadWrapper, this);
}

void Worker::_SignalHandler(int) {
	NETWORK_CURRENT_PROCESS_DEBUG("Signal handler was activated from Stop() function");
}

// See Worker.h
void Worker::Stop() {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);
	if (_current_state.load() == STATE::STOPPED) { return; }
	if (_current_state.load() == STATE::STOPPING) { Join(); }

	_current_state.store(STATE::STOPPING);
	pthread_kill(_thread.native_handle(), SIGUSR1); //Send signal to stop epoll
	Join();
	_current_state.store(STATE::STOPPED);
}

// See Worker.h
void Worker::Join() {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);
	if (_thread.joinable()) { _thread.join(); }
}

void Worker::_ThreadWrapper() {
	try	{
		_ThreadFunction();
	}
	catch (std::exception& exc) {
		NETWORK_CURRENT_PROCESS_DEBUG("EXCEPTION in thread (process will be stopped): " << exc.what());
		_clients.clear();
	}
}

bool Worker::_ReadFromSocket(int epoll, ClientAndExecutor& client_executor) {
	std::string str;
	auto io_information = client_executor.client.Receive(str);
	while (io_information.state == Core::FileDescriptor::IO_OPERATION_STATE::OK) {
		if (io_information.result == 0) { return false; } //Socket was closed

		if (client_executor.executor.AppendAndTryExecute(str)) {
			epoll_event socket_event = {};
			socket_event.data.fd = client_executor.client.GetID();
			socket_event.events = EPOLLIN | EPOLLOUT;
			VALIDATE_NETWORK_FUNCTION(epoll_ctl(epoll, EPOLL_CTL_MOD, client_executor.client.GetID(), &socket_event));
		}
		io_information = client_executor.client.Receive(str);
	}
	if (io_information.state == Core::FileDescriptor::IO_OPERATION_STATE::ASYNC_ERROR) { return true; }
	else { return false; }
}

bool Worker::_WriteToSocket(int epoll, ClientAndExecutor& client_executor) {
	while (client_executor.executor.HasOutputData()) {
		auto io_information = client_executor.client.Send(client_executor.executor.GetOutputAsIovec(), client_executor.executor.GetQueueSize());
		if (io_information.state == Core::FileDescriptor::IO_OPERATION_STATE::ASYNC_ERROR) { return true; }
		if (io_information.state == Core::FileDescriptor::IO_OPERATION_STATE::ERROR) { return false; }

		client_executor.executor.RemoveFromOutput(io_information.result);
	}
	
	epoll_event socket_event = {};
	socket_event.data.fd = client_executor.client.GetID();
	socket_event.events = EPOLLIN;
	VALIDATE_NETWORK_FUNCTION(epoll_ctl(epoll, EPOLL_CTL_MOD, client_executor.client.GetID(), &socket_event));
	return true;
}

// See Worker.h
void Worker::_ThreadFunction() {
	NETWORK_CURRENT_PROCESS_DEBUG(__PRETTY_FUNCTION__);

    // TODO: implementation here
    // 1. Create epoll_context here
    // 2. Add server_socket to context
    // 3. Accept new connections, don't forget to call make_socket_nonblocking on
    //    the client socket descriptor
    // 4. Add connections to the local context
    // 5. Process connection events
    //
    // Do not forget to use EPOLLEXCLUSIVE flag when register socket
    // for events to avoid thundering herd type behavior.

	int epoll = -1;
	VALIDATE_NETWORK_FUNCTION(epoll = epoll_create1(0));

	epoll_event socket_event = {};
	socket_event.data.fd = _server_socket->GetID();
	socket_event.events = EPOLLIN | EPOLLEXCLUSIVE;
	VALIDATE_NETWORK_FUNCTION(epoll_ctl(epoll, EPOLL_CTL_ADD, _server_socket->GetID(), &socket_event));

	epoll_event* events = (epoll_event*) calloc(_max_listeners + 1, sizeof(epoll_event)); //+1 - for server socket
	if (events == nullptr) {
		throw std::runtime_error("Cannot calloc() memory for events!");
	}
	
	while (_current_state.load() == STATE::WORKS) {
		int n = epoll_wait(epoll, events, _max_listeners + 1, -1);
		if (n == -1) {
			if (errno == EINTR && _current_state.load() != STATE::WORKS) { break; } //Worker is stopping
			else {
				throw NetworkException("EPoll wait failed!");
			}
		}

		if (_current_state.load() != STATE::WORKS) { break; } //Server is stopping
		for (int i = 0; i < n; i++) {
			if (events[i].data.fd == _server_socket->GetID()) {
				VALIDATE_NETWORK_CONDITION(events[i].events & EPOLLIN); //Only epollin is a correct event
				auto accept_information = _server_socket->Accept();
				VALIDATE_NETWORK_CONDITION(accept_information.state == Core::FileDescriptor::IO_OPERATION_STATE::OK); // No async errors are allowed

				accept_information.socket.MakeNonblocking();
				socket_event.data.fd = accept_information.socket.GetID();
				socket_event.events = EPOLLIN;
				VALIDATE_NETWORK_FUNCTION(epoll_ctl(epoll, EPOLL_CTL_ADD, accept_information.socket.GetID(), &socket_event));
				_clients.emplace(std::make_pair(accept_information.socket.GetID(), ClientAndExecutor(std::move(accept_information.socket), _storage)));
			}
			else {
				VALIDATE_NETWORK_CONDITION(events[i].events & EPOLLIN || events [i].events & EPOLLOUT || events [i].events & EPOLLHUP || 
							   events[i].events & EPOLLERR); //Events mask
				auto client = _clients.find(events[i].data.fd);
				if (client == _clients.end()) { continue; } //Unknown or unregistered client in thread pool

				if (events[i].events & EPOLLHUP || events [i].events & EPOLLERR) { //Socket was closed
					_clients.erase(client); //Remove socket from listening
					continue;
				}

				auto client_executor = &(_clients.find(events[i].data.fd)->second);
				if (events[i].events & EPOLLIN) {
					if (!_ReadFromSocket(epoll, *client_executor)) {
						_clients.erase(client);
						continue;
					}
				}
				if (events[i].events & EPOLLOUT) {
					if (!_WriteToSocket(epoll, *client_executor)) {
						_clients.erase(client);
						continue;
					}
				}
			}
		}
	}
	
	_clients.clear();
	free(events);
}

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
