#include<iostream>

#include "ThreadPool.h"

//#define LOCK_THREADPOOL_MUTEX std::unique_lock<std::mutex> __lock(threadpool_mutex)

namespace Afina {
namespace Core {

ThreadPool::ThreadPool() : _low_watermark(0), _hight_watermark(0), _max_queue_size(0), _idle_time(0), 
						   state(ThreadPool::State::kStopped), _count_free_threads(0)
{}

ThreadPool::~ThreadPool() {
	Stop(true);
}

void ThreadPool::_ThreadFunction() {
	THREADPOOL_CURRENT_PROCESS_DEBUG(__PRETTY_FUNCTION__);
	try {
		bool thread_unregistered = false;
		while (state.load() == ThreadPool::State::kRun) {
			_ExecuteTasks();
			if (_idle_time != 0) { 
				if (_TryUnregisterThread()) { //Checks low watermark
					thread_unregistered = true;
					break; 
				}
			}
		}
		if (!thread_unregistered) {
			if (!_TryUnregisterThread()) {
				throw std::runtime_error("Cannot unregister thread after server was stopped!");
			}
		}
	}
	catch (std::exception& exc) {
		THREADPOOL_CURRENT_PROCESS_DEBUG("EXCEPTION in thread (process will be stopped): " << exc.what());
	}
	THREADPOOL_CURRENT_PROCESS_DEBUG(__PRETTY_FUNCTION__ << " was finished");
}

void ThreadPool::_ExecuteTasks() {
	//Waiting for new tasks
	while (tasks.empty() && state.load() == ThreadPool::State::kRun) {
		std::unique_lock<std::mutex> lock(threadpool_mutex); //Will be released in wait_for()/wait() function
		++_count_free_threads;
		if (_idle_time != 0) {
			auto status = empty_condition.wait_for(lock, std::chrono::milliseconds(_idle_time));
			if (status == std::cv_status::timeout) //No need wait more
			{ 
				--_count_free_threads;
				break; 
			}
		}
		else {
			empty_condition.wait(lock);
		}
	    --_count_free_threads; //Waiting was finished
	}

	//Extract the new tasks
	while (!tasks.empty()) {
		std::deque<std::function<void()>>::iterator task_iterator;
		std::function<void()> task;
		//Extract task under mutex
		{
			std::unique_lock<std::mutex> __lock(threadpool_mutex);
			task_iterator = tasks.begin();
			if (task_iterator == tasks.end()) { return; } //No new tasks
			task = std::move(*task_iterator);
			tasks.erase(task_iterator);
		}

		try { //Other problems are system problems in thread and it should be finished - so try-catch is in _ThreadFunction()
			task(); //Execue
		}
		catch (std::exception& exc) {
			THREADPOOL_CURRENT_PROCESS_DEBUG("EXCEPTION during the execution of the task: " << exc.what());
		}
	}
}

bool ThreadPool::_TryUnregisterThread() {
	std::unique_lock<std::mutex> __lock(threadpool_mutex);
	if (threads.size() <= _low_watermark + 1 && state.load() == ThreadPool::State::kRun) { return false; } //Cannot finish due to low watermark

	auto current_thread_iterator = threads.find(std::this_thread::get_id());
	if (current_thread_iterator == threads.end()) {
		throw std::runtime_error("Thread pool tries to finish unregistered thread");
	}
	current_thread_iterator->second.detach();
	threads.erase(current_thread_iterator);

	//Set thread pool state if it was the last thread
	if (state.load() == ThreadPool::State::kStopping && threads.empty()) {
		state.store(ThreadPool::State::kStopped);
	}

	return true;
}

void ThreadPool::_StartThread(bool need_lock) {
	if (need_lock) { threadpool_mutex.lock(); }

	auto new_thread = std::thread(&ThreadPool::_ThreadFunction, this);
	threads.emplace(new_thread.get_id(), std::move(new_thread));

	if (need_lock) { threadpool_mutex.unlock(); }
}

void ThreadPool::Start(size_t low_watermark, size_t hight_watermark, size_t max_queue_size, unsigned int idle_time) {
	THREADPOOL_DEBUG(__PRETTY_FUNCTION__);

	_low_watermark = low_watermark;
	_hight_watermark = hight_watermark;
	_max_queue_size = max_queue_size;
	_idle_time = idle_time;

	if (hight_watermark < low_watermark) {
		throw std::invalid_argument("hight_watermark < low_watermark in thread pool!");
	}

	std::unique_lock<std::mutex> __lock(threadpool_mutex);
	for (int i = 0; i < _low_watermark; i++) {
		//move semantic
		_StartThread(false);
	}
	state.store(ThreadPool::State::kRun);
}

void ThreadPool::Stop(bool await) {
	THREADPOOL_DEBUG(__PRETTY_FUNCTION__);
	if (state.load() != ThreadPool::State::kRun) { return; }
	state.store(ThreadPool::State::kStopping);

	//Wake up all threads that are waiting new tasks
	empty_condition.notify_all();

	if (await) {
		while (!threads.empty()) {
			_ThreadsIterator next_thread;
			{ 
				std::unique_lock<std::mutex> __lock(threadpool_mutex);
				next_thread = threads.begin();
				if (next_thread == threads.end()) { break; } //All threads was finished before we get unique lock
			} //Free lock to get a allow thread to finish

			try {
				if (next_thread->second.joinable()) { next_thread->second.join(); } 
			}
			catch(std::system_error& exc) {
				if (exc.code() == std::errc::no_such_process) { continue; } //The process has finished before we've called join()
			}
		}
		state.store(ThreadPool::State::kStopped); //Only if we are waiting we can guarantee stop state
	}

        THREADPOOL_DEBUG(__PRETTY_FUNCTION__ << " finished");	
}

} // namespace Afina
} // namespace Core
