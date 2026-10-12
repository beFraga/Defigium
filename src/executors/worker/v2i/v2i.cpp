#ifdef BUILD_V2I_STRATEGY

#include "v2i.h"
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>

V2IExecutorStrategy::V2IExecutorStrategy() = default;
V2IExecutorStrategy::~V2IExecutorStrategy() = default;

void V2IExecutorStrategy::connect(std::string& ip, int port) {

}

ExecutionResult V2IExecutorStrategy::execute(const Command& command) {

}

#endif
