#include "Daemon.h"
#include "Utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>
#include <sstream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <sys/file.h>
#include <fcntl.h>

bool is_safe_token(const std::string& s, const std::string& extra_allowed) {
	if (s.empty()) return false;
	for (unsigned char c : s)
		if (!std::isalnum(c) && extra_allowed.find(c) == std::string::npos)
			return false;
	return true;
}

Daemon::Daemon(int port) : daemon_port(port), worker_pid(-1) {
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		printerr("Critical Error: Socket creation failed.");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(daemon_port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        printerr("Critical Error: Bind failed on port " + std::to_string(daemon_port));
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        printerr("Critical Error: Listen failed.");
        exit(EXIT_FAILURE);
    }
}

Daemon::~Daemon() {
	kill_worker();
	if (server_fd >= 0) close(server_fd);
}

void Daemon::run() {
	print("Listening for Master on port " + std::to_string(daemon_port) + "...");

	while (true) {
		int client_sock = accept(server_fd, nullptr, nullptr);
		if (client_sock < 0) continue;

		print("Master connected. Awaiting commands...");
		std::thread(&Daemon::handle_client, this, client_sock).detach();
	}
}

void Daemon::handle_client(int client_sock) {
    char buffer[512];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) {
            print("Master disconnected.");
            break;
        }

        std::istringstream iss(buffer);
        std::string command;
        iss >> command;

		if (command == "WAKE") {
            std::string type, bin_path, target_ip;
            int threads, target_port;
			long long t0;
			if (iss >> type >> bin_path >> target_ip >> target_port >> threads >> t0) {
				int reserved_fd = -1;
				int port = reserve_ephemeral_port(reserved_fd);
				if (port != -1) {
					// Pass reserved_fd so spawn_worker can close it after fork,
					// keeping the port "claimed" until the worker binds it.
					spawn_worker(client_sock, port, reserved_fd, type, bin_path, threads, target_ip, target_port, t0);
				} else {
					printerr("Critical Error: No available ports found for worker.");
					std::string err_msg = "FAILED\n";
					send(client_sock, err_msg.c_str(), err_msg.length(), 0);
				}
			}
		} 
        else if (command == "KILL") {
            kill_worker();
        }
    }
    
    close(client_sock);
}

void Daemon::spawn_worker(int client_sock, int port, int reserved_fd, const std::string& type, const std::string& bin_path, int threads, const std::string& target_ip, int target_port, long long t0) {
    kill_worker();

	if (!is_safe_token(type, "_") || !is_safe_token(target_ip, ".:")) {
		printerr("Refusing to spawn worker: unsafe TYPE or target IP in WAKE command.");
		return;
	}
	
	char cwd[1024];
	if (!getcwd(cwd, sizeof(cwd))) {
		printerr("Critical Error: Could not get current working directory.");
		return;
	}

	print("Starting to wake worker...");
	
	std::string worker_dir = std::string(cwd) + "/../worker";
	std::string build_dir = bin_path + "/build_worker";
	std::string state_file = build_dir + "/.current_type";

	std::string mkdir_cmd = "mkdir -p " + build_dir;
	print(mkdir_cmd);
	system(mkdir_cmd.c_str());

	bool need_rebuild = true;
	{
		std::ifstream file(state_file);
		if (file.is_open()) {
			std::string saved_type;
			std::getline(file, saved_type);
			if (saved_type == type) need_rebuild = false;
		}
	}

	int lock_fd = -1;
	if (need_rebuild) {
		std::string lock_path = build_dir + "/.build.lock";
		lock_fd = open(lock_path.c_str(), O_CREAT | O_RDWR, 0644);
		if (lock_fd < 0) {
			printerr("Critical Error: Could not open build lock file.");
			return;
		}

		print("Waiting for build lock on " + lock_path + "...");
		if (flock(lock_fd, LOCK_EX) < 0) {
			printerr("Critical Error: Could not acquire build lock.");
			close(lock_fd);
			return;
		}

		std::ifstream file(state_file);
		if (file.is_open()) {
			std::string saved_type;
			std::getline(file, saved_type);
			if (saved_type == type) need_rebuild = false;
		}
	}

	if (need_rebuild) {
		std::ostringstream cmd_build;
		print("Type changed or build missing. Rebuilding worker for type: " + type);
		cmd_build << "cmake -S " << worker_dir << " -B" << build_dir 
			<< " -DTYPE=" << type << " > /dev/null 2>&1 && "
			<< "cmake --build " << build_dir << " -j > /dev/null 2>&1 && "
			<< "echo " << type << " > " << state_file;
		int build_res = system(cmd_build.str().c_str());
		print(std::to_string(build_res));
		if (build_res != 0) {
			printerr("Worker build failed.");
			return;
		}
		print("Worker build completed successfully.");
	}

	if (lock_fd >= 0) {
		flock(lock_fd, LOCK_UN);
		close(lock_fd);
	}

    pid_t pid = fork();
    if (pid < 0) {
        printerr("Failed to fork worker process.");
        if (reserved_fd >= 0) close(reserved_fd);
        return;
    }

    if (pid == 0) {
		// Child: close reserved_fd before exec so the worker can bind the port
		if (reserved_fd >= 0) close(reserved_fd);

		std::ostringstream cmd;
		cmd << "exec " << build_dir << "/worker "
			<< port << " " << threads << " " << type << " "
			<< target_ip << " " << target_port << " " << t0;

        execlp("/bin/sh", "sh", "-c", cmd.str().c_str(), nullptr);
        
        printerr("Execution of worker build/run failed!");
        exit(EXIT_FAILURE); 
    } else {
        // Parent: release our hold on the reserved port now that the child is alive.
        // Worker will bind it with SO_REUSEADDR; no other daemon can steal it anymore
        // because the window between reserve and fork is now closed.
        if (reserved_fd >= 0) close(reserved_fd);

        worker_pid = pid;
        print("Successfully spawned Worker PID " + std::to_string(pid) + " waiting 3s to respond to Master.");

		std::string ready_msg = "OK " + std::to_string(port) + "\n";
		std::this_thread::sleep_for(std::chrono::seconds(5));
		send(client_sock, ready_msg.c_str(), ready_msg.length(), 0);
    }
}

void Daemon::kill_worker() {
    if (worker_pid <= 0) return;

    print("Terminating active worker PID " + std::to_string(worker_pid) + "...");
    
    kill(worker_pid, SIGTERM);
    waitpid(worker_pid, nullptr, 0); 
    
    print("Worker terminated.");
    worker_pid = -1;
}

// Returns a race-free ephemeral port by binding to port 0 and letting the OS
// assign one atomically. The returned fd is already bound — caller MUST close
// it after fork() so the child (worker) can bind the same port via SO_REUSEADDR.
int Daemon::reserve_ephemeral_port(int& out_fd) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return -1;

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(0); // Let OS pick a free port atomically

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(sock);
		return -1;
	}

	socklen_t len = sizeof(addr);
	if (getsockname(sock, (struct sockaddr*)&addr, &len) < 0) {
		close(sock);
		return -1;
	}

	out_fd = sock; // Caller owns this fd — must close after fork
	return ntohs(addr.sin_port);
}

int Daemon::get_available_cores() {
    unsigned int cores = std::thread::hardware_concurrency();
    return cores > 0 ? cores : 1;
}
