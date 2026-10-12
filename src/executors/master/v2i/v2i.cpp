#ifdef BUILD_V2I_STRATEGY

#include "v2i.h"
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>

V2IParserStrategy::V2IParserStrategy() = default;
V2IParserStrategy::~V2IParserStrategy() = default;

std::vector<std::string> V2IParserStrategy::parse_command_args(const std::string& command_str) {

}

std::optional<Task> V2IParserStrategy::parse_line(const std::string& log_line) {

}

#endif
