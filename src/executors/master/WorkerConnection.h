#pragma once

#include <string>
#include <vector>
#include <map>
#include <sys/epoll.h>
#include "interfaces.h"
#include "Connection.h"

enum class WorkerState {
	CONNECTING, READY,
	WAIT_BARRIER, BARRIER_REACHED,
	RUNNING, DONE, FAILED
};

class WorkerConnection : public Connection {
public:
    WorkerConnection(const std::string & ip, int port)
		: Connection(ip, port), state(WorkerState::CONNECTING) {}

	void on_write() override {
		if (state == WorkerState::CONNECTING) {
			int err = 0;
			socklen_t len = sizeof(err);
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);

			if (err == 0)
				state = WorkerState::READY;
			else
				on_error();
		}
	}

	void on_read() override {
		std::string incoming;
		if (receive_data(incoming) <= 0) {
			on_error();
			return;
		}

		read_buffer += incoming;

		if (state == WorkerState::WAIT_BARRIER) {
			size_t pos = read_buffer.find("BARRIER_REACHED");
			if (pos == std::string::npos) return;

			state = WorkerState::BARRIER_REACHED;
			read_buffer.erase(0, pos + 15);
		}
		else if (state == WorkerState::RUNNING) {
			if (read_buffer.find("END_RESULTS") == std::string::npos) return;

			state = WorkerState::DONE;
			read_buffer.clear();
		}
	}

	void on_error() override {
		state = WorkerState::FAILED;
		close_socket();
	}

	WorkerState get_state() const { return state; }
	void set_state(WorkerState s) { state = s; }

	std::string get_results() const { return final_results; }

	bool is_listening_for_read() const { return listening_for_read; }
	void set_listening_for_read(bool v) { listening_for_read = v; }

	bool is_resolved() const { return resolved; }
	void set_resolved(bool v) { resolved = v; }

private:
	WorkerState state;
	std::string final_results;

	bool listening_for_read = false;
	bool resolved = false;
};
