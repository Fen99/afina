#ifndef AFINA_THREADPOOL_H
#define AFINA_THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>

#include "Debug.h"

namespace Afina {
namespace Core {

/**
* # Thread pool
*/
class ThreadPool
{
public:
	enum class State
	{
		// Threadpool is fully operational, tasks could be added and get executed
		kRun,

		// Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
		// completed as requested
		kStopping,

		// Threadppol is stopped
		kStopped
	};

	ThreadPool();
	~ThreadPool();

	/**
	* Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
	* free. All enqueued jobs will be complete.
	*
	* In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
	*/
	void Stop(bool await = false);

	// Starts low_watermark of threads
	void Start(size_t low_watermark = 0, size_t hight_watermark = 10, size_t max_queue_size = 20, unsigned int idle_time = 0);

	State GetState() const { return state; }

	/**
	* Add function to be executed on the threadpool. Method returns true in case if task has been placed
	* onto execution queue, i.e scheduled for execution and false otherwise.
	*
	* That function doesn't wait for function result. Function could always be written in a way to notify caller about
	* execution finished by itself
	*/
	template <typename F, typename... Types>
	bool Execute(F&& func, Types... args)
	{
		// Prepare "task"
		auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);
		if (state.load() != State::kRun) { return false; }

		std::unique_lock<std::mutex> lock(threadpool_mutex);
		// Enqueue new task
		if (tasks.size() >= _max_queue_size) { return false; } //Cannot create new task
		tasks.push_back(exec);
		if (_count_free_threads.load() == 0 && threads.size() < _hight_watermark) {
   		    _StartThread(false); 
		}

		empty_condition.notify_one();
		return true;
	}

private:
	// No copy/move/assign allowed
	ThreadPool(const ThreadPool&)            = delete;
	ThreadPool(ThreadPool&&)                 = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&)      = delete;

	/**
		* Main function that all pool threads are running. It polls internal task queue and execute tasks (calls _ExecuteTasks)
	*/
	//friend void perform(ThreadPool* pool);
	void _ThreadFunction();

	// Executes tasks in thread
	void _ExecuteTasks();

	// Starts a new thread
	void _StartThread(bool need_lock);

	// Returns true if thread was removed from threads map, false if it cannot be finished due to low_watermark
	bool _TryUnregisterThread();

	/**
	* Mutex to protect state below from concurrent modification
	*/
	std::mutex threadpool_mutex;

	/**
	* Conditional variable to await new data in case of empty queue
	*/
	std::condition_variable empty_condition;

	/**
	* Unordered map of actual threads that perorm execution
	* thread_id is the key
	*/
	std::unordered_map<std::thread::id, std::thread> threads;
	using _ThreadsIterator = std::unordered_map<std::thread::id, std::thread>::iterator;
	std::atomic<unsigned int> _count_free_threads;

	/**
	* Task queue
	*/
	std::deque<std::function<void()>> tasks;

	/**
	* Flag to stop bg threads
	*/
	std::atomic<State> state;

	size_t _low_watermark;
	size_t _hight_watermark;
	size_t _max_queue_size;
	unsigned int _idle_time;
};

} // namespace Core
} // namespace Afina

#endif // AFINA_THREADPOOL_H
