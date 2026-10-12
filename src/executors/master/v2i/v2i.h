#pragma once
#include "../interfaces.h"
#include <memory>
#include <regex>

class V2IParserStrategy : public IParserStrategy {
public:
    V2IParserStrategy();
    ~V2IParserStrategy();

    std::optional<Task> parse_line(const std::string& log_line) override;

private:
    std::vector<std::string> parse_command_args(const std::string& command_str);
};
