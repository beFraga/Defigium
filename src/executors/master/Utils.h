#pragma once
#include <iostream>
#include <string>
#include "interfaces.h"

void print(const std::string& msg);
void printerr(const std::string& msg);

std::string serialize_task(long long ts_ns, const Command& cmd);
