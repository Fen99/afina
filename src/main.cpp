#include <chrono>
#include <iostream>
#include <memory>
#include <uv.h>

#include <cxxopts.hpp>

#include <afina/Storage.h>
#include <afina/Version.h>
#include <afina/network/Server.h>

#include "pipes/FIFOServer.h"

#include "network/blocking/ServerImpl.h"
#include "network/nonblocking/ServerImpl.h"
#include "network/uv/ServerImpl.h"
#include "storage/MapBasedGlobalLockImpl.h"

typedef struct {
    std::shared_ptr<Afina::Storage> storage;
    std::shared_ptr<Afina::Network::Server> server;
	std::shared_ptr<Afina::FIFONamespace::FIFOServer> fifo;
} Application;

// Handle all signals catched
void signal_handler(uv_signal_t *handle, int signum) {
    Application *pApp = static_cast<Application *>(handle->data);

    std::cout << "Receive stop signal" << std::endl;
    uv_stop(handle->loop);
}

// Called when it is time to collect passive metrics from services
void timer_handler(uv_timer_t *handle) {
    Application *pApp = static_cast<Application *>(handle->data);
    std::cout << "Start passive metrics collection" << std::endl;
}

int main(int argc, char **argv) {
    // Build version
    // TODO: move into Version.h as a function
    std::stringstream app_string;
    app_string << "Afina " << Afina::Version_Major << "." << Afina::Version_Minor << "." << Afina::Version_Patch;
    if (Afina::Version_SHA.size() > 0) {
        app_string << "-" << Afina::Version_SHA;
    }

    // Command line arguments parsing
    cxxopts::Options options("afina", "Simple memory caching server");
    try {
        // TODO: use custom cxxopts::value to print options possible values in help message
        // and simplify validation below
        options.add_options()("s,storage", "Type of storage service to use", cxxopts::value<std::string>());
        options.add_options()("n,network", "Type of network service to use", cxxopts::value<std::string>());
		options.add_options()("r,read", "Reading FIFO name", cxxopts::value<std::string>());
		options.add_options()("w,write", "Writing FIFO name", cxxopts::value<std::string>());
        options.add_options()("h,help", "Print usage info");
        options.parse(argc, argv);

        if (options.count("help") > 0) {
            std::cerr << options.help() << std::endl;
            return 0;
        }
    } catch (cxxopts::OptionParseException &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    // Start boot sequence
    Application app;
    std::cout << "Starting " << app_string.str() << std::endl;

    // Build new storage instance
    std::string storage_type = "map_global";
    if (options.count("storage") > 0) {
        storage_type = options["storage"].as<std::string>();
    }

    if (storage_type == "map_global") {
        app.storage = std::make_shared<Afina::Backend::MapBasedGlobalLockImpl>();
    } else {
        throw std::runtime_error("Unknown storage type");
    }

    // Build  & start network layer
    std::string network_type = "uv";
    if (options.count("network") > 0) {
        network_type = options["network"].as<std::string>();
    }

    if (network_type == "uv") {
        app.server = std::make_shared<Afina::Network::UV::ServerImpl>(app.storage);
    } else if (network_type == "blocking") {
        app.server = std::make_shared<Afina::Network::Blocking::ServerImpl>(app.storage);
    } else if (network_type == "nonblocking") {
        app.server = std::make_shared<Afina::Network::NonBlocking::ServerImpl>(app.storage);
    } else {
        throw std::runtime_error("Unknown network type");
    }

	// Init FIFO
	std::string reading_fifo_name;
	std::string writing_fifo_name;
	if (options.count("read") > 0) {
		app.fifo = std::make_shared<Afina::FIFONamespace::FIFOServer>(app.storage);
		reading_fifo_name = options["read"].as<std::string>();
		if (options.count("write") > 0) {
			writing_fifo_name = options["write"].as<std::string>();
		}
	}

    // Init local loop. It will react to signals and performs some metrics collections. Each
    // subsystem is able to push metrics actively, but some metrics could be collected only
    // by polling, so loop here will does that work
    uv_loop_t loop;
    uv_loop_init(&loop);

	uv_signal_t sig_term, sig_int;
	uv_signal_init(&loop, &sig_term);
	uv_signal_init(&loop, &sig_int);
	uv_signal_start(&sig_term, signal_handler, SIGTERM);
	uv_signal_start(&sig_int, signal_handler, SIGINT);
	sig_term.data = &app;
	sig_int.data = &app;

    uv_timer_t timer;
    uv_timer_init(&loop, &timer);
    timer.data = &app;
    uv_timer_start(&timer, timer_handler, 0, 5000);

    // Start services
    try {
        app.storage->Start();
        app.server->Start(8080);
	if (app.fifo != nullptr) { app.fifo->Start(reading_fifo_name, writing_fifo_name); }

        // Freeze current thread and process events
        std::cout << "Application started" << std::endl;
        uv_run(&loop, UV_RUN_DEFAULT);

        // Stop services
        app.server->Stop();
        app.server->Join();
	if (app.fifo != nullptr) {
		app.fifo->Stop();
		app.fifo->Join();
	}
        app.storage->Stop();

        std::cout << "Application stopped" << std::endl;
    } catch (std::exception &e) {
        std::cerr << "Fatal error" << e.what() << std::endl;
    }

    return 0;
}
