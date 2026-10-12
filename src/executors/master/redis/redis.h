#pragma once
#include "../interfaces.h"
#include <memory>
#include <regex>

namespace sw { namespace redis { class Redis; } }

class RedisParserStrategy : public IParserStrategy {
public:
    RedisParserStrategy();
    ~RedisParserStrategy();

    std::optional<Task> parse_line(const std::string& log_line) override;

private:
    std::vector<std::string> parse_command_args(const std::string& command_str);
};
