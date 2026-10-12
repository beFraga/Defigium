#pragma once
#include <iostream>
#include <string>
#include <optional>
#include "interfaces.h"

void print(const std::string & msg);
void printerr(const std::string & msg);

std::optional<Task> deserialize_task(const std::string& line);
