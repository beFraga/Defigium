#pragma once
#include "interfaces.h"
#include <string>
#include <memory>

class ParserFactory {
public:
    static std::unique_ptr<IParserStrategy> create(const std::string& type);
};
