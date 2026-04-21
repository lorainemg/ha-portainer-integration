#pragma once

#include <string>
#include "../dashboard/stack.h"

// Reads a YAML config file and returns the full state (approved stacks + ignored list).
YamlState loadYamlState(const std::string& filepath);

// Writes the YAML state back to the given path, overwriting it.
// Empty optional fields (containers, ignored_containers, ignored) are omitted from output.
void saveYamlState(const std::string& filepath, const YamlState& state);
