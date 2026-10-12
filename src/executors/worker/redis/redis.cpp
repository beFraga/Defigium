#ifdef BUILD_REDIS_STRATEGY

#include "redis.h"
#include <sw/redis++/redis++.h>
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>

RedisExecutorStrategy::RedisExecutorStrategy() = default;
RedisExecutorStrategy::~RedisExecutorStrategy() = default;

void RedisExecutorStrategy::connect(std::string& ip, int port) {
    sw::redis::ConnectionOptions connection_options;
    connection_options.host = ip;
    connection_options.port = port;

    connection_options.socket_timeout = std::chrono::seconds(1);

    connection_options.connect_timeout = std::chrono::seconds(1);

    m_redis_client = std::make_unique<sw::redis::Redis>(connection_options);
}

ExecutionResult RedisExecutorStrategy::execute(const Command& command) {
	std::chrono::steady_clock::time_point start_time, end_time;
	bool timing_started = false;
	

    try {
        const auto& args_map = command.additional_data;
        const auto it = args_map.find("raw_args");
        const std::vector<std::string> raw_args = (it != args_map.end()) ? it->second : std::vector<std::string>();

        if (command.op_type == "HMSET") {
            if (!raw_args.empty() && raw_args.size() % 2 == 0) {
                std::vector<std::pair<std::string, std::string>> field_values;
                for (size_t i = 0; i < raw_args.size(); i += 2) {
                    field_values.emplace_back(raw_args[i], raw_args[i + 1]);
                }
				start_time = std::chrono::steady_clock::now();
				timing_started = true;
                m_redis_client->hmset(command.target, field_values.begin(), field_values.end());
				end_time = std::chrono::steady_clock::now();

            }
        } else if (command.op_type == "SET") {
            if (!raw_args.empty()) {
				start_time = std::chrono::steady_clock::now();
				timing_started = true;
                m_redis_client->set(command.target, raw_args[0]);
				end_time = std::chrono::steady_clock::now();
            }
        } else if (command.op_type == "GET") {
			start_time = std::chrono::steady_clock::now();
			timing_started = true;
            m_redis_client->get(command.target);
			end_time = std::chrono::steady_clock::now();
        } else if (command.op_type == "HGETALL") {
            std::vector<std::pair<std::string, std::string>> result;
			start_time = std::chrono::steady_clock::now();
			timing_started = true;
            m_redis_client->hgetall(command.target, std::back_inserter(result));
			end_time = std::chrono::steady_clock::now();
        } else if (command.op_type == "DEL") {
			start_time = std::chrono::steady_clock::now();
			timing_started = true;
            m_redis_client->del(command.target);
			end_time = std::chrono::steady_clock::now();
        } else if (command.op_type == "ZADD") {
            if (raw_args.size() >= 2) {
				start_time = std::chrono::steady_clock::now();
				timing_started = true;
                m_redis_client->zadd(command.target, raw_args[1], std::stod(raw_args[0]));
				end_time = std::chrono::steady_clock::now();
            }
        }
        // For more commands, add additional else-if blocks here.
		long long latency_ns = 0;
		if (timing_started) {
			auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
			latency_ns = elapsed.count();
		}
        return {latency_ns, true};
    } catch (const std::exception& e) {
        std::stringstream ss;
		ss << "ERRO ao executar comando [" << command.op_type << "] no alvo [" << command.target << "]: " << e.what() << std::endl;
		std::cerr << ss.str();
        return {0, false};
    }
}

#endif
