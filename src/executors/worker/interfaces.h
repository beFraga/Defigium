#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <chrono>

struct Command {
    std::string op_type;
    std::string target;
    std::string client_id;
    std::map<std::string, std::vector<std::string>> additional_data;
};

struct Task {
    long long original_timestamp;
    Command command;
};

struct ExecutionResult {
    long long latency_ns;
    bool success;
};

class IExecutorStrategy {
public:
    virtual ~IExecutorStrategy() = default;
    virtual void connect(std::string& ip, int port) = 0;
    virtual ExecutionResult execute(const Command& command) = 0;
};
