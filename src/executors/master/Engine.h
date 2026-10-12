#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "Config.h"
#include "EventLoop.h"
#include "DaemonConnection.h"
#include "WorkerConnection.h"

class Engine {
	public:
		Engine(Config * config);
		~Engine();

		void init();
	private:
		Config::Nodes _daemons;
		Config::Node _target;
		std::string _file_path;
		std::string _bin_path;
		int _num_threads;
		std::string _target_type;
		std::ifstream _file;

		std::unique_ptr<IParserStrategy> _parser;
		EventLoop event_loop;
		std::vector<std::unique_ptr<DaemonConnection>> daemon_conn;
		std::vector<std::unique_ptr<WorkerConnection>> worker_conn;

		bool wake_workers();
		bool handshake();
		void schedule();
		void run_barrier();
		void get_results();
		void output_file();
		void termination();

		long long milli_to_nano(double t);
};
