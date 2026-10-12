#include "Utils.h"
#include <sstream>

void print(const std::string & msg) { std::cout << "[Worker] " << msg << std::endl; }
void printerr(const std::string & msg) { std::cerr << "[Worker] " << msg << std::endl; }

static bool deserialize_str(std::istream& is, std::string& s) {
	size_t len = 0;
	char colon = 0;
	if (!(is >> len >> colon) || colon != ':') return false;
	s.resize(len);
	if (len > 0) {
		is.read(&s[0], len);
	}
	return !is.fail();
}

std::optional<Task> deserialize_task(const std::string& line) {
	std::istringstream iss(line);
	std::string prefix;
	if (!(iss >> prefix) || prefix != "TASK") return std::nullopt;

	long long ts = 0;
	if (!(iss >> ts)) return std::nullopt;

	Command cmd;
	if (!deserialize_str(iss, cmd.op_type)) return std::nullopt;
	if (!deserialize_str(iss, cmd.target)) return std::nullopt;
	if (!deserialize_str(iss, cmd.client_id)) return std::nullopt;

	size_t num_keys = 0;
	if (!(iss >> num_keys)) return std::nullopt;

	for (size_t i = 0; i < num_keys; ++i) {
		std::string key;
		if (!deserialize_str(iss, key)) return std::nullopt;

		size_t num_vals = 0;
		if (!(iss >> num_vals)) return std::nullopt;

		std::vector<std::string> vals;
		vals.reserve(num_vals);
		for (size_t j = 0; j < num_vals; ++j) {
			std::string val;
			if (!deserialize_str(iss, val)) return std::nullopt;
			vals.push_back(std::move(val));
		}
		cmd.additional_data[key] = std::move(vals);
	}

	return Task{ts, std::move(cmd)};
}
