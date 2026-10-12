#include "Engine.h"
#include "Utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sstream>

Engine::Engine(int port, int num_threads, std::string& type, std::string& tip, int tport, long long t0)
	: sockfd(-1),
	  _port(port),
	  _t0(t0),
	  pool(num_threads, tip, tport, type) {
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(_port);
}

Engine::~Engine() {
	if (sockfd >= 0) close(sockfd);
}

void Engine::init() {
	handshake();

	receive_data();

	pool.wait_for_connections();

	prepare_tasks();

	run_barrier();

	auto sync_start_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
	pool.set_start_time(sync_start_time);

	pool.start_execution();
	pool.wait_for_completion();

	print("Execution completed. Sending END_RESULTS to Master...");
	std::string end_msg = "END_RESULTS\n";
	send(sockfd, end_msg.c_str(), end_msg.length(), 0);

	print("Ending execution...");
}

void Engine::handshake() {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(_port);

	bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	listen(server_fd, 1);

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	print("Waiting for Master connection on port " + std::to_string(_port));

	sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	close(server_fd);

	print("Master connected. Handshake complete.");
}

void Engine::receive_data() {
	print("Waiting for task injection...");
	std::string buffer;
	char buf[65536];
	bool receiving = true;

	while (receiving) {
		ssize_t bytes_rcv = recv(sockfd, buf, sizeof(buf), 0);

		if (bytes_rcv <= 0) {
			printerr("Connection to Master lost during data reception.");
			break;
		}

		buffer.append(buf, bytes_rcv);

		size_t pos = 0;
		while ((pos = buffer.find('\n')) != std::string::npos) {
			std::string line = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);

			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			if (line.rfind("TASK ", 0) == 0) {
				if (auto task_opt = deserialize_task(line)) {
					tasks.push_back(std::move(*task_opt));
				} else {
					printerr("Warning: Malformed task line received: " + line);
				}
			}
			else if (line == "END_TASKS") {
				receiving = false;
				print("Received " + std::to_string(tasks.size()) + " tasks.");
				break;
			}
		}
	}
}

void Engine::prepare_tasks() {
	if (tasks.empty()) return;

	std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
		return a.original_timestamp < b.original_timestamp;
	});

	for (const auto& task : tasks) {
		Task_Worker tw;
		tw.command = task.command;
		long long delay_ns = task.original_timestamp >= _t0 ? (task.original_timestamp - _t0) : 0;
		tw.delay_offset = std::chrono::nanoseconds(delay_ns);
		pool.enqueue_task(std::move(tw));
	}
}

void Engine::run_barrier() {
	print("Notifying Master: BARRIER_REACHED...");
	std::string msg = "BARRIER_REACHED\n";
	ssize_t sent = send(sockfd, msg.c_str(), msg.length(), 0);
	if (sent < 0) {
		printerr("Failed to send BARRIER_REACHED to Master.");
		return;
	}

	print("Waiting for RELEASE_BARRIER from Master...");
	std::string buffer;
	char buf[256];
	while (true) {
		ssize_t bytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
		if (bytes <= 0) {
			printerr("Connection lost while waiting for RELEASE_BARRIER.");
			return;
		}
		buffer.append(buf, bytes);
		if (buffer.find("RELEASE_BARRIER") != std::string::npos) {
			print("Barrier released by Master. Synchronizing start time...");
			break;
		}
	}
}
