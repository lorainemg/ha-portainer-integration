#include "config_parser.h"
#include "../errors.h"
#include <yaml-cpp/yaml.h>

// Helper to check a required field exists and return its value
static std::string requireField(const YAML::Node& node,
                                const std::string& field,
                                const std::string& context) {
    if (!node[field]) {
        throw ConfigError(context + " missing required field '" + field + "'");
    }
    return node[field].as<std::string>();
}

std::vector<Stack> loadStacks(const std::string& filepath) {
    YAML::Node yamlData;
    try {
        yamlData = YAML::LoadFile(filepath);
    } catch (const YAML::BadFile&) {
        throw ConfigError("Could not open config file: " + filepath);
    } catch (const YAML::Exception& e) {
        throw ConfigError("Failed to parse YAML config: " + std::string(e.what()));
    }

    if (!yamlData["stacks"] || !yamlData["stacks"].IsSequence()) {
        throw ConfigError("Config file missing required 'stacks' list");
    }

    std::vector<Stack> stacks;
    for (size_t i = 0; i < yamlData["stacks"].size(); ++i) {
        const auto& stack = yamlData["stacks"][i];
        std::string ctx = "Stack #" + std::to_string(i + 1);

        auto stackObj = Stack{
            .slug = requireField(stack, "slug", ctx),
            .name = requireField(stack, "name", ctx),
            .icon = requireField(stack, "icon", ctx),
            .portainer_id = stack["portainer_id"].as<int>(0),
        };

        if (stack["containers"]) {
            std::vector<Container> containers;
            for (size_t j = 0; j < stack["containers"].size(); ++j) {
                const auto& container = stack["containers"][j];
                std::string cctx = ctx + ", container #" + std::to_string(j + 1);

                containers.push_back(Container{
                    .slug = requireField(container, "slug", cctx),
                    .name = requireField(container, "name", cctx),
                    .icon = requireField(container, "icon", cctx)});
            }
            stackObj.containers = containers;
        }
        stacks.push_back(stackObj);
    }
    return stacks;
}
