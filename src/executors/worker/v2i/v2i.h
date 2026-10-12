#pragma once
#include "../interfaces.h"
#include <memory>
#include <regex>

class V2IExecutorStrategy : public IExecutorStrategy {
public:
    V2IExecutorStrategy();
    ~V2IExecutorStrategy();

    void connect(std::string& ip, int port) override;
    ExecutionResult execute(const Command& command) override;
    std::optional<Task> parse_line(const std::string& log_line) override;

private:
    std::vector<std::string> parse_command_args(const std::string& command_str);
};
