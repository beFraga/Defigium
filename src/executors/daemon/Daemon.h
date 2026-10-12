#pragma once

#include <string>
#include <atomic>
#include <netinet/in.h>
#include <sys/types.h>

class Daemon {
	public:
		Daemon(int port, int log_fd);
		~Daemon();

		void run();

	private:
		int server_fd;
		int daemon_port;
		int log_fd;

		pid_t worker_pid;

		void handle_client(int client_sock);
		void spawn_worker(int client_sock, int port, int reserved_fd, const std::string& type, const std::string& bin_path, int threads, const std::string& target_ip, int target_port, long long t0);

		int reserve_ephemeral_port(int& out_fd);
		void kill_worker();
		int get_available_cores();
};
