#ifdef BUILD_REDIS_STRATEGY

#include "redis.h"
#include <sw/redis++/redis++.h>
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>

RedisParserStrategy::RedisParserStrategy() = default;
RedisParserStrategy::~RedisParserStrategy() = default;

std::vector<std::string> RedisParserStrategy::parse_command_args(const std::string& command_str) {
    std::vector<std::string> args;
    std::string current_arg;
    bool in_quotes = false;

    size_t i = 0;
    while (i < command_str.length()) {
        char c = command_str[i];

        if (!in_quotes) {
            if (c == '"') {
                in_quotes = true;
            }
            i++;
            continue;
        }

        bool is_end_of_string = (i + 1 == command_str.length());
        bool is_separator = (i + 2 < command_str.length() && command_str[i+1] == ' ' && command_str[i+2] == '"');

        if (c == '"' && (is_end_of_string || is_separator)) {
            in_quotes = false;
            args.push_back(current_arg);
            current_arg.clear();
        } else {
            current_arg += c;
        }
        i++;
    }
    return args;
}


std::optional<Task> RedisParserStrategy::parse_line(const std::string& log_line) {
    static const std::regex line_splitter_regex(R"(^(\S+)\s+\[([^\]]+)\]\s+(.*)\s*$)");

    std::smatch line_parts;
    if (!std::regex_match(log_line, line_parts, line_splitter_regex)) {
        std::cout << "Parse FAILED: Line does not match basic structure.\n" << std::endl;
        return std::nullopt;
    }

    Task task;
    try {
        task.original_timestamp = std::stod(line_parts[1].str());
    } catch (const std::invalid_argument&) {
        std::cout << "Parse FAILED: Invalid timestamp.\n" << std::endl;
        return std::nullopt;
    }

    task.command.client_id = line_parts[2].str();

    std::vector<std::string> all_args = parse_command_args(line_parts[3].str());

    if (all_args.empty()) {
        std::cout << "Parse FAILED: No arguments found.\n" << std::endl;
        return std::nullopt;
    }

    task.command.op_type = all_args[0];
    if (all_args.size() > 1) {
        task.command.target = all_args[1];
    }
    if (all_args.size() > 2) {
        task.command.additional_data["raw_args"] = {all_args.begin() + 2, all_args.end()};
    }

    return task;
}

#endif
