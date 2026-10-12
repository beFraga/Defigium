#pragma once

#include "Config.h"
#include "Connection.h"
#include "Utils.h"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <string>
#include <vector>
#include <map>
#include <utility>

enum class DaemonState { CONNECTING, WAIT_WAKE, DONE, FAILED };

class DaemonConnection : public Connection {
public:
	DaemonConnection(const std::string & ip, int port, std::string launch_cmd)
		: Connection(ip, port), state(DaemonState::CONNECTING), cmd(launch_cmd) {}


	void on_read() override {
		if (state == DaemonState::WAIT_WAKE) {
			std::string incoming;
			if (receive_data(incoming) <= 0) {
				on_error();
				return;
			}

			read_buffer += incoming;
			size_t ok_pos = read_buffer.find("OK ");
			if (ok_pos == std::string::npos) return;

			try {
				std::string port_str = read_buffer.substr(ok_pos + 3);
				worker_port = std::stoi(port_str);
				state = DaemonState::DONE;
				read_buffer.clear();
			} catch (...) {
				on_error();
			}
		}
	}

	void on_write() override {
		if (state == DaemonState::CONNECTING) {
			int err = 0;
			socklen_t len = sizeof(err);
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);

			if (err == 0) {
				state = DaemonState::WAIT_WAKE;
				send_data(cmd);
			} else {
				on_error();
			}
		}
	}

	void on_error() override {
		state = DaemonState::FAILED;
		close_socket();
	}

	int get_worker_port() const { return worker_port; }
	DaemonState get_state() const { return state; }

	bool is_listening_for_read() const { return listening_for_read; }
	void set_listening_for_read(bool v) { listening_for_read = v; }

	bool is_resolved() const { return resolved; }
	void set_resolved(bool v) { resolved = v; }

private:
	DaemonState state;
	std::string cmd;
	int worker_port;

	bool listening_for_read = false;
	bool resolved = false;
};
