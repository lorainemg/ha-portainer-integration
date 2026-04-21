#pragma once

#include <string>
#include "../dashboard/stack.h"

// Reads a YAML config file and returns the full state (approved stacks + ignored list).
YamlState loadYamlState(const std::string& filepath);
