#include "Utils.h"
#include <sstream>

void print(const std::string& msg) { std::cout << "[Master] " << msg << std::endl; }
void printerr(const std::string& msg) { std::cerr << "[Master] " << msg << std::endl; }

static void serialize_str(std::ostream& os, const std::string& s) {
	os << s.size() << ":" << s << " ";
}

std::string serialize_task(long long ts_ns, const Command& cmd) {
	std::ostringstream oss;
	oss << "TASK " << ts_ns << " ";
	serialize_str(oss, cmd.op_type);
	serialize_str(oss, cmd.target);
	serialize_str(oss, cmd.client_id);
	oss << cmd.additional_data.size() << " ";
	for (const auto& [key, vals] : cmd.additional_data) {
		serialize_str(oss, key);
		oss << vals.size() << " ";
		for (const auto& v : vals) {
			serialize_str(oss, v);
		}
	}
	oss << "\n";
	return oss.str();
}
