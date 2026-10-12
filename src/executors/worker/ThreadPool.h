#pragma once

#include "interfaces.h"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <string>
#include <chrono>
#include <iostream>
#include <memory>

struct Task_Worker {
    std::chrono::nanoseconds delay_offset;
    Command command;
};

template <typename T>
class ThreadSafeQueue {
public:
	void push(T value) {
		std::lock_guard<std::mutex> lock(_mutex);
		_queue.push(std::move(value));
		_cond.notify_one();
	}
	T pop() {
		std::unique_lock<std::mutex> lock(_mutex);
		_cond.wait(lock, [this]{ return !_queue.empty(); });
		T value = std::move(_queue.front());
		_queue.pop();
		return value;
	}

	bool empty() {
		std::lock_guard<std::mutex> lock(_mutex);
		return _queue.empty();
	}
private:
	std::queue<T> _queue;
	std::mutex _mutex;
	std::condition_variable _cond;
};

class ThreadPool {
public:
	ThreadPool(size_t num_threads, std::string& tip, int tport, std::string& ttype);
	~ThreadPool();

	void set_start_time(std::chrono::steady_clock::time_point start_time);
	void start_execution();

	void enqueue_task(Task_Worker task);
	bool is_work_complete();
	void wait_for_connections();
	void wait_for_completion();

	std::atomic<long long> success_count{0};
	std::atomic<long long> error_count{0};
	std::atomic<long long> total_latency_ns{0};

private:
	void worker_function(int id);
	
	size_t _num_workers;
	std::string _tip;
	int _tport;
	std::string _ttype;

	ThreadSafeQueue<Task_Worker> _queue;
	std::vector<std::thread> _workers;

	std::mutex _init_mutex;
	std::condition_variable _init_cond;
	size_t _connected_workers = 0;

	std::chrono::steady_clock::time_point _start_time;
	std::mutex _start_mutex;
	std::condition_variable _start_cond;
	bool _started = false;
	bool _completed = false;
};
