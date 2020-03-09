#ifndef AFINA_FIFO_SERVER_H
#define AFINA_FIFO_SERVER_H

#include <atomic>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "./../protocol/Executor.h"
#include "core/FIFO.h"
#include <afina/Storage.h>

namespace Afina {
namespace FIFONamespace {

class FIFOServer {
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

    void Start(const std::string &reading_name, const std::string &writing_name);
    void Stop();
    void Join();
};

} // namespace FIFONamespace
} // namespace Afina

#endif // AFINA_FIFO_SERVER_H
