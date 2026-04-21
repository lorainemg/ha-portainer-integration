#pragma once
#include <string>

// Plain data — what the Portainer API actually returns (subset we care about).
struct PortainerStack {
    int id = 0;
    std::string name;        // human name from Portainer (e.g. "Home Assistant")
};

struct PortainerContainer {
    std::string name;        // docker container name without leading '/'
    std::string stack_name;  // value of com.docker.compose.project label; "" if standalone
    int stack_id = 0;        // populated later by reconciler via name lookup
};
