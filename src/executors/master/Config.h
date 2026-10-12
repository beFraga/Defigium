#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class Config {
	public:
		struct Node {
			std::string ip;
			int port;

			Node() : ip(""), port(0) {}
			Node(const std::string& ip, int port) : ip(ip), port(port) {}
		};

		using Nodes = std::vector<Node>;

		Config(const std::string& path_config);

		// Re-resolve to a different named benchmark without re-reading the file.
		void resolve(const std::string& benchmark_name);

		int get_num_threads() { return _num_threads; }
		Node get_target() { return _target; }
		std::string get_type() { return _type; }
		std::string get_file_path() { return _file_path; }
		std::string get_bin_path() { return _bin_path; }
		Nodes get_nodes() { return _nodes; }

		// Returns the ordered sequence of benchmark names to run.
		// Falls back to { active_benchmark } when benchmark_sequence is absent/empty.
		std::vector<std::string> get_benchmark_sequence() { return _benchmark_sequence; }

	private:
		std::string _path_config;
		YAML::Node  _raw;             // full parsed YAML, kept for resolve()
		int _num_threads;
		Node _target;
		std::string _type;
		std::string _file_path;
		std::string _bin_path;
		Nodes _nodes;
		std::vector<std::string> _benchmark_sequence;

		void _apply(const YAML::Node& config);
		std::string filepath_concat(const std::string base_path, const std::string number);
};
