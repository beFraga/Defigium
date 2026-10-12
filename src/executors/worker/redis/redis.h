#pragma once
#include "../interfaces.h"
#include <memory>
#include <regex>

namespace sw { namespace redis { class Redis; } }

class RedisExecutorStrategy : public IExecutorStrategy {
public:
    RedisExecutorStrategy();
    ~RedisExecutorStrategy();

    void connect(std::string& ip, int port) override;
    ExecutionResult execute(const Command& command) override;

private:
    std::unique_ptr<sw::redis::Redis> m_redis_client;
};
