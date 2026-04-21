#pragma once

#include <string>
#include <vector>

// Plain data holders — struct because everything is public (like @dataclass)

struct Container {
    std::string slug;
    std::string name;
    std::string icon;
};

struct Stack {
    std::string slug;
    std::string name;
    std::string icon;
    int portainer_id = 0;
    std::vector<Container> containers;  // dynamic array, like Python's list
    std::vector<std::string> ignored_containers;  // slugs of declined containers within this stack

    // Helper methods matching your Python @property methods

    // Returns true if this stack has child containers (expander view)
    bool isExpander() const { return !containers.empty(); }

    // Builds the Portainer URL for this stack
    std::string portainerUrl(const std::string& portainer_base) const {
        if (portainer_id == 0) return "";
        // Replace underscores with hyphens for the URL
        std::string url_name = slug;
        for (char& c : url_name) {
            if (c == '_') c = '-';
        }
        return portainer_base + "/#!/3/docker/stacks/" + url_name
             + "?id=" + std::to_string(portainer_id)
             + "&type=2&regular=true&orphaned=false&orphanedRunning=false";
    }

    // Entity ID helpers
    std::string status() const { return "binary_sensor." + slug + "_status"; }
    std::string stackSwitch() const { return "switch." + slug + "_stack"; }
};

// Whole-file representation of stacks.yaml.
//   stacks  — approved stacks (rendered on the dashboard)
//   ignored — slugs of declined whole stacks (skipped on future runs)
struct YamlState {
    std::vector<Stack> stacks;
    std::vector<std::string> ignored;
};
