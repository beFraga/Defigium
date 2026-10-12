#include "Engine.h"
#include "DaemonConnection.h"
#include "WorkerConnection.h"
#include "factory.h"
#include <sys/epoll.h>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <chrono>

Engine::Engine(Config * config) {
	_daemons	 = config->get_nodes();
	_file_path   = config->get_file_path();
	_bin_path	 = config->get_bin_path();
	_num_threads = config->get_num_threads();
	_target_type = config->get_type();
	_target		 = config->get_target();
	_parser		 = ParserFactory::create(_target_type);
	_file.open(_file_path);
}

Engine::~Engine() {
	if (_file.is_open()) _file.close();
}

void Engine::init() {
	print("Initializing orchestration sequence..");
	// Connect with daemon
	// Check who are up and receive num_cores
	// Send port, type, and num threads (create a logic to choose the number of threads for each node) for each worker to daemon
	// IMPORTANT: not all workers need to be up, if the number of threads be 2, you just need 2 (or 1) worker in it
	// Daemon will wake workers with these parameters
	// Connect with workers (Same ip as daemon but in other port)
	if (!wake_workers()) return;
	// handshake() -> Check if they are up
	if (!handshake()) return;

	// schedule() -> Send data with socket by some schedule algorithm (like round robin) it will pass all file sending data to all workers equally
	schedule();
	// run_barrier() -> wait workers preprocess data, and when all are done fire the benchmark
	run_barrier();
	// get_results() -> wait them finish and get results
	get_results();
	// output_file() -> with these statistics make a file (maybe .txt) with all data about benchmark
	output_file();
	// termination() -> send to daemon a msg to kill executors and end task
	termination();
}

bool Engine::wake_workers() {
	print("Connecting to Daemons...");

	int active_daemons = _daemons.size();
	if (active_daemons == 0) return false;

	int base_threads = _num_threads / active_daemons;
	int remainder = _num_threads % active_daemons;

	if (!_file.is_open()) {
		printerr("Error: Could not open synthetic log file: " + _file_path);
		return false;
	}

	std::string line;
	std::optional<Task> task;
	while (!task && std::getline(_file, line))
		task = _parser->parse_line(line);
	if (!task) {
		printerr("Error: synthetic log file is empty or not available for the requested type.");
		return false;
	}
	long long t0 = milli_to_nano(task->original_timestamp);
	print("T0: " + std::to_string(t0));

	for (const auto& [ip, port] : _daemons) {

		int assigned_threads = base_threads + (remainder > 0 ? 1 : 0);
		if (remainder > 0) remainder--;

		std::string launch_cmd = "WAKE "
			+ _target_type + " "
			+ _bin_path + " "
			+ _target.ip + " "
			+ std::to_string(_target.port) + " "
			+ std::to_string(assigned_threads) + " "
			+ std::to_string(t0) + "\n";

		auto conn = std::make_unique<DaemonConnection>(ip, port, launch_cmd);

		if (conn->connect_non_blocking()) {
			event_loop.register_connection(conn.get(), EPOLLOUT | EPOLLET);
			daemon_conn.push_back(std::move(conn));
		}
	}

	int pending = daemon_conn.size();
	if (pending == 0) return false;

	while (pending > 0) {
		event_loop.run_once(5000);

		for (auto& conn : daemon_conn) {
			DaemonState state = conn->get_state();

			if (state == DaemonState::WAIT_WAKE && !conn->is_listening_for_read()) {
				event_loop.modify_connection(conn.get(), EPOLLIN | EPOLLET);
				conn->set_listening_for_read(true);
			}

			else if (state == DaemonState::DONE || state == DaemonState::FAILED) {
				if (conn->is_resolved()) continue;

				conn->set_resolved(true);
				pending--;
				if (state == DaemonState::DONE) print("Daemon " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " ready.");
				else printerr("Daemon " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " failed.");
			}
		}
	}

	for (const auto& conn : daemon_conn)
		if (conn->get_state() == DaemonState::DONE) return true;
	print("Critical Error: Failed to successfully wake workers on any daemons!");
	return false;
}

bool Engine::handshake() {
	print("Initializing handshake with workers...");

	for (const auto & d_conn : daemon_conn) {
		if (d_conn->get_state() != DaemonState::DONE) continue;

		auto w_conn = std::make_unique<WorkerConnection>(d_conn->get_ip(), d_conn->get_worker_port());
		if (!w_conn->connect_non_blocking()) continue;

		event_loop.register_connection(w_conn.get(), EPOLLOUT | EPOLLET);
		worker_conn.push_back(std::move(w_conn));
	}

	int pending = worker_conn.size();
	if (pending == 0) return false;

	while (pending > 0) {
		event_loop.run_once(5000);

		for (auto & conn : worker_conn) {
			WorkerState state = conn->get_state();
			print(std::to_string(static_cast<int>(state)));
			if (!(state == WorkerState::READY || state == WorkerState::FAILED) || conn->is_resolved()) continue;

			conn->set_resolved(true);
			pending--;

			if (state == WorkerState::READY) print("Worker " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " is ready.");
			else print("Worker " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " failed to connect.");
		}
	}

	for (const auto & conn : worker_conn)
		if (conn->get_state() == WorkerState::READY) return true;
	return true;
}

void Engine::schedule() {
	//std::vector<Task> all_tasks = parse_log_file(_file_path);
	print("Parsing tasks and distributing to active workers...");

	std::vector<WorkerConnection *> active_w;
	for (auto & wc : worker_conn)
		if (wc->get_state() == WorkerState::READY)
			active_w.push_back(wc.get());

	if (active_w.empty()) {
		printerr("Critical Error: No active workers available for scheduling.");
		return;
	}

	_file.clear();
	_file.seekg(0, std::ios::beg);

	std::vector<std::string> payloads(active_w.size(), "");
	std::string line;
	size_t task_idx = 0;

	while (std::getline(_file, line)) {
		if (auto task = _parser->parse_line(line)) {
			int w_idx = task_idx++ % active_w.size();
			long long ts_ns = milli_to_nano(task->original_timestamp);
			payloads[w_idx] += serialize_task(ts_ns, task->command);
		}
	}

	if (task_idx == 0) {
		printerr("Critical Error: No valid tasks found in log file.");
		return;
	}

	print("Distributed " + std::to_string(task_idx) + " tasks.");

	int pending = active_w.size();

	for (size_t i = 0; i < active_w.size(); ++i) {
		payloads[i] += "END_TASKS\n";

		active_w[i]->set_state(WorkerState::WAIT_BARRIER);
		active_w[i]->send_data(payloads[i]);

		event_loop.modify_connection(active_w[i], EPOLLIN | EPOLLET);
		active_w[i]->set_resolved(false);
	}

	print("Waiting for all workers to reach synchronization barrier...");

	while (pending > 0) {
		event_loop.run_once(5000);

		for (auto * conn : active_w) {
			WorkerState state = conn->get_state();
			if (!(state == WorkerState::BARRIER_REACHED || state == WorkerState::FAILED) || conn->is_resolved()) continue;

			conn->set_resolved(true);
			pending--;

			if (state == WorkerState::BARRIER_REACHED)
				print("Worker " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " reached barrier.");
			else
				printerr("Worker " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " failed during scheduling.");
		}
	}
}

void Engine::run_barrier() {
	print("All workers synchronized. Broadcasting RELEASE_BARRIER...");
	for (auto & conn : worker_conn) {
		if (conn->get_state() != WorkerState::BARRIER_REACHED) continue;

		conn->send_data("RELEASE_BARRIER\n");

		conn->set_state(WorkerState::RUNNING);
		conn->set_resolved(false);
	}
}

void Engine::get_results() {
	std::vector<WorkerConnection *> running_w;
	for (auto & conn : worker_conn)
		if (conn->get_state() == WorkerState::RUNNING)
			running_w.push_back(conn.get());

	if (running_w.empty()) {
		print("No running workers to wait for.");
		return;
	}

	print("Waiting for all workers to finish execution...");

	int pending = running_w.size();
	for (auto * conn : running_w)
		conn->set_resolved(false);

	while (pending > 0) {
		event_loop.run_once(30000);

		for (auto * conn : running_w) {
			WorkerState state = conn->get_state();
			if (!(state == WorkerState::DONE || state == WorkerState::FAILED) || conn->is_resolved()) continue;

			conn->set_resolved(true);
			pending--;

			if (state == WorkerState::DONE)
				print("Worker " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " finished execution.");
			else
				printerr("Worker " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + " failed during execution.");
		}
	}

	print("All workers have reported completion.");
}

void Engine::output_file() {
}

void Engine::termination() {
	print("Sending KILL to all daemons...");

	for (auto & conn : daemon_conn) {
		if (conn->get_state() != DaemonState::DONE) continue;

		std::string kill_cmd = "KILL\n";
		conn->send_data(kill_cmd);
		print("KILL sent to daemon " + conn->get_ip() + ":" + std::to_string(conn->get_port()) + ".");
	}

	print("Termination protocol complete. Daemons remain active for future rounds.");
}

long long Engine::milli_to_nano(double t) {
	std::chrono::duration<double, std::milli> src_dur(t);
	auto nano_dur = std::chrono::duration_cast<std::chrono::nanoseconds>(src_dur);
	return nano_dur.count();
}
