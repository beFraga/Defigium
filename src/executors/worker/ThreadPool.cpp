#include "ThreadPool.h"
#include "factory.h"
#include "Utils.h"

ThreadPool::ThreadPool(size_t num_workers, std::string& tip, int tport, std::string& ttype)
	: _num_workers(num_workers), _tip(tip), _tport(tport), _ttype(ttype)
{
	_workers.reserve(num_workers);
	print("Threads connecting to target...");
	for (size_t i = 0; i < num_workers; ++i)
		_workers.emplace_back(&ThreadPool::worker_function, this, i);
}

ThreadPool::~ThreadPool() {
	wait_for_completion();
}

void ThreadPool::wait_for_completion() {
	if (_completed) return;
	_completed = true;

	for (size_t i = 0; i < _num_workers; ++i) {
		Task_Worker poison;
		poison.delay_offset = std::chrono::nanoseconds::zero();
		poison.command.op_type = "POISON_PILL";
		_queue.push(poison);
	}

	start_execution();

	for (std::thread &worker : _workers)
		if (worker.joinable())
			worker.join();
}

void ThreadPool::set_start_time(std::chrono::steady_clock::time_point start_time) {
	_start_time = start_time;
}

void ThreadPool::start_execution() {
	{
		std::lock_guard<std::mutex> lock(_start_mutex);
		_started = true;
	}
	_start_cond.notify_all();
}

void ThreadPool::enqueue_task(Task_Worker task) {
	_queue.push(std::move(task));
}

bool ThreadPool::is_work_complete() {
	return _queue.empty();
}

void ThreadPool::wait_for_connections() {
	std::unique_lock<std::mutex> lock(_init_mutex);
	_init_cond.wait(lock, [this] { return _connected_workers == _num_workers; });
}

void ThreadPool::worker_function(int id) {
	std::unique_ptr<IExecutorStrategy> executor = nullptr;

    try {
        executor = ExecutorFactory::create(_ttype);
        executor->connect(_tip, _tport);

		{
			std::lock_guard<std::mutex> lock(_init_mutex);
			_connected_workers++;
			_init_cond.notify_one();
		}

		{
			std::unique_lock<std::mutex> lock(_start_mutex);
			_start_cond.wait(lock, [this] { return _started; });
		}

        while (true) {
            Task_Worker task = _queue.pop();
            if (task.command.op_type == "POISON_PILL") break;

			std::this_thread::sleep_until(_start_time + task.delay_offset);

            ExecutionResult result = executor->execute(task.command);

            if (result.success) {
                success_count++;
				auto cur_lat = total_latency_ns.load();
				while(!total_latency_ns.compare_exchange_weak(cur_lat, cur_lat + result.latency_ns));
            } else {
                error_count++;
            }
        }
    } catch (const std::exception &e) {
        printerr("Error in Thread " + std::to_string(id) + ": " + e.what());
        error_count++;
		{
			std::lock_guard<std::mutex> lock(_init_mutex);
			_connected_workers++;
			_init_cond.notify_one();
		}
    }

    if (executor) {
        executor.reset();
    }
}

