#pragma once

#include <string>
#include <vector>
#include "../dashboard/stack.h"

// Reads a YAML config file and returns the stack definitions
std::vector<Stack> loadStacks(const std::string& filepath);
