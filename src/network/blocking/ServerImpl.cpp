#include "ServerImpl.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <pthread.h>
#include <signal.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <afina/Storage.h>

#define LOCK_CONNECTIONS_MUTEX std::lock_guard<std::mutex> __lock(connections_mutex)

const int reading_portion_g = 1024;

namespace Afina {
namespace Network {
namespace Blocking {

// See Server.h
ServerImpl::ServerImpl(std::shared_ptr<Afina::Storage> ps) :
	Server(ps), _server_socket(-1), running(false), _is_finishing(false), listen_port(0), _thread_pool()
{}

// See Server.h
ServerImpl::~ServerImpl() 
{
	//Check if all sockets closed
	Stop();
}

// See Server.h
void ServerImpl::Start(uint16_t port, uint16_t n_workers) {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);

	//Check if server was started yet
	if (running.load() == true) {
		throw std::runtime_error("Cannot use ServerImpl::Start() if server was started yet!");
	}

    // If a client closes a connection, this will generally produce a SIGPIPE
    // signal that will kill the process. We want to ignore this signal, so send()
    // just returns -1 when this happens.
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &sig_mask, NULL) != 0) {
        throw std::runtime_error("Unable to mask SIGPIPE");
    }

    // Setup server parameters BEFORE thread created, that will guarantee
    // variable value visibility
    listen_port = port;

    // The pthread_create function creates a new thread.
    //
    // The first parameter is a pointer to a pthread_t variable, which we can use
    // in the remainder of the program to manage this thread.
    //
    // The second parameter is used to specify the attributes of this new thread
    // (e.g., its stack size). We can leave it NULL here.
    //
    // The third parameter is the function this thread will run. This function *must*
    // have the following prototype:
    //    void *f(void *args);
    //
    // Note how the function expects a single parameter of type void*. We are using it to
    // pass this pointer in order to proxy call to the class member function. The fourth
    // parameter to pthread_create is used to specify this parameter value.
    //
    // The thread we are creating here is the "server thread", which will be
    // responsible for listening on port 23300 for incoming connections. This thread,
    // in turn, will spawn threads to service each incoming connection, allowing
    // multiple clients to connect simultaneously.
    // Note that, in this particular example, creating a "server thread" is redundant,
    // since there will only be one server thread, and the program's main thread (the
    // one running main()) could fulfill this purpose.
    _thread_pool.Start(0, n_workers);
    
    running.store(true);
    if (pthread_create(&accept_thread, NULL, ServerImpl::RunMethodInDifferentThread<&ServerImpl::RunAcceptor>, this) < 0) {
        throw std::runtime_error("Could not create server thread");
    }
}

// See Server.h
void ServerImpl::Stop() {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);

	if (!running.load() || _is_finishing.load()) { return; }
	
	_is_finishing.store(true);
	// Shutdown listening socket and all client sockets
	// sockets will be closed by their threads
	{
		LOCK_CONNECTIONS_MUTEX;
		for (auto it = _client_sockets.begin(); it != _client_sockets.end(); it++) {
			shutdown(*it, SHUT_RDWR); 
		}
	}
	_thread_pool.Stop(true);

	shutdown(_server_socket, SHUT_RDWR);
	pthread_join(accept_thread, 0);

	running.store(false);
	_is_finishing.store(false);
	connections_cv.notify_all();
}

// See Server.h
void ServerImpl::Join() {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);
    //pthread_join(accept_thread, 0); - incorrect! Accepted thread can be finished earlyer (e.g., system err.) then thread for connections

	while (running.load()) {
		//mutex will be used only for signaling
		std::unique_lock<std::mutex> lock(connections_mutex);
		connections_cv.wait(lock);
	}
}

// See Server.h
void ServerImpl::RunAcceptor() {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);

    // For IPv4 we use struct sockaddr_in:
    // struct sockaddr_in {
    //     short int          sin_family;  // Address family, AF_INET
    //     unsigned short int sin_port;    // Port number
    //     struct in_addr     sin_addr;    // Internet address
    //     unsigned char      sin_zero[8]; // Same size as struct sockaddr
    // };
    //
    // Note we need to convert the port to network order
    sockaddr_in server_addr = {};
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;          // IPv4
    server_addr.sin_port = htons(listen_port); // TCP port number
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any address

    // Arguments are:
    // - Family: IPv4
    // - Type: Full-duplex stream (reliable)
    // - Protocol: TCP
    _server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server_socket == -1) {
        throw std::runtime_error("Failed to open socket");
    }

    // when the server closes the socket,the connection must stay in the TIME_WAIT state to
    // make sure the client received the acknowledgement that the connection has been terminated.
    // During this time, this port is unavailable to other processes, unless we specify this option
    //
    // This option let kernel knows that we are OK that multiple threads/processes are listen on the
    // same port. In a such case kernel will balance input traffic between all listeners (except those who
    // are closed already)
    int opts = 1;
    if (setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts)) == -1) {
        close(_server_socket);
        throw std::runtime_error("Socket setsockopt() failed");
    }

    // Bind the socket to the address. In other words let kernel know data for what address we'd
    // like to see in the socket
    if (bind(_server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        close(_server_socket);
        throw std::runtime_error("Socket bind() failed");
    }

    // Start listening. The second parameter is the "backlog", or the maximum number of
    // connections that we'll allow to queue up. Note that listen() (!)doesn't block until
    // incoming connections arrive. It just makes the OS aware that this process is willing
    // to accept connections on this socket (which is bound to a specific IP and port)
    if (listen(_server_socket, max_listen) == -1) {
        close(_server_socket);
        throw std::runtime_error("Socket listen() failed");
    }

	int client_socket = -1;
	sockaddr_in client_addr = {};
	std::memset(&client_addr, 0, sizeof(client_addr));
	socklen_t sinSize = sizeof(sockaddr_in);
    while (running.load() && !_is_finishing.load()) {
        	std::cout << "network debug: waiting for connection..." << std::endl;

		// When an incoming connection arrives, accept it. The call to accept() blocks until
		// the incoming connection arrives
		client_socket = accept(_server_socket, (sockaddr *) &client_addr, &sinSize);
		if (client_socket == -1) {
			if (_is_finishing.load()) { break; } //No exception is needed
			close(_server_socket);
			throw std::runtime_error("Socket accept() failed.");
		}
		NETWORK_DEBUG("Connection accepted from address: 0x" << std::hex << client_addr.sin_addr.s_addr);
		
		//Check limit

		{
			//Block mutex for work with connections set (_thread_pool.Execute starts a function immediatly, but _client_sockets.insert should be performed
			LOCK_CONNECTIONS_MUTEX; 
			if (!_thread_pool.Execute(&ServerImpl::RunConnection, this, client_socket)) {
				std::string message = "SERVER_ERROR Server is buisy an cannot accept a new client\r\n";
				if (send(client_socket, message.data(), message.size(), 0) <= 0) {
					close(client_socket); //Closes only client socket
				}
				NETWORK_DEBUG("Connection was rejected due to _thread_pool.Execute = false");
				continue;
			}
			_client_sockets.insert(client_socket);
		}
    }

    // Cleanup on exit...
    close(_server_socket);
}

// See Server.h
void ServerImpl::RunConnection(int client_socket) {
	NETWORK_DEBUG(__PRETTY_FUNCTION__);
    // TODO: All connection work is here

	// TODO: Start new thread and process data from/to connection
	Afina::Protocol::Parser parser;
	std::string current_data;
	while (running.load()) {
		char new_data [reading_portion_g] = "";
		if (recv(client_socket, new_data, reading_portion_g * sizeof(char), 0) <= 0) { break; }
		current_data.append(new_data);
		memset(new_data, 0, reading_portion_g *sizeof(char)); //No set '\0' in recv function
		
		size_t parsed = 0;
		bool was_command = false;
		try {
			was_command = parser.Parse(current_data, parsed);
		}
		catch(std::exception& e) {
			std::string error_msg = "Parsing error: ";
			error_msg += e.what();
			error_msg += "\r\n";
			send(client_socket, error_msg.c_str(), error_msg.size(), 0);
			current_data = "";
			parser.Reset();
			continue;
		}
		current_data = current_data.substr(parsed); //remove command from received data
		if (!was_command) { continue; } //more data is needed
		
		//if command was accepted
		uint32_t read_for_arg = 0;
		auto command = parser.Build(read_for_arg);
		if (read_for_arg != 0) { read_for_arg += 2; } //\r\n
		if (read_for_arg > current_data.size()) { //we need to read some more for argument. Not need if no argument is needed
			if (recv(client_socket, new_data, (read_for_arg) * sizeof(char), MSG_WAITALL) <= 0) {
				NETWORK_CURRENT_PROCESS_DEBUG("Server hasn't received argument from client before the socket was closed");
				break;
			}
			current_data.append(new_data);
		}
		std::string argument;
		if (read_for_arg > 2) {
			argument = current_data.substr(0, read_for_arg-2); // \r\n not needed
			current_data = current_data.substr(read_for_arg); //remove argument from received data
		}

		std::string out;
		try {
			command->Execute(*pStorage, argument, out);
		}
		catch(std::exception& e) {
			out = "SERVER ERROR ";
			out += e.what();
		}
		out += "\r\n";
		size_t len_sended = send(client_socket, out.c_str(), out.size(), 0);
		if (len_sended < out.size()) {
			NETWORK_CURRENT_PROCESS_DEBUG("Server cannot send all data to client");
			break;
		}

		parser.Reset();
	}

	close(client_socket);
	//Removes this client from list
	{
		LOCK_CONNECTIONS_MUTEX;
		_client_sockets.erase(_client_sockets.find(client_socket));
	}
	NETWORK_PROCESS_DEBUG(pthread_self(), "Connection to client was closed");	
}

} // namespace Blocking
} // namespace Network
} // namespace Afina
