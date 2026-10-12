#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <netinet/in.h>
#include "ThreadPool.h"
#include "interfaces.h"

class Engine {
	public:
		Engine(int port, int num_threads, std::string& type, std::string& tip, int tport, long long t0);
		~Engine();

		void init();
	private:
		int sockfd;
		struct sockaddr_in address;
		int _port;
		long long _t0;

		ThreadPool pool;
		std::vector<Task> tasks;

		void handshake();
		void receive_data();
		void run_barrier();
		void prepare_tasks();
};
