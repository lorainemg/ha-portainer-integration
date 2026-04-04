#include "config_parser.h"
#include <yaml-cpp/yaml.h>

std::vector<Stack> loadStacks(const std::string& filepath) {
    auto yamlData = YAML::LoadFile(filepath);
    std::vector<Stack> stacks;
    for (const auto& stack : yamlData["stacks"]) {
        auto stackObj = Stack{
            .slug = stack["slug"].as<std::string>(),
            .name = stack["name"].as<std::string>(),
            .icon = stack["icon"].as<std::string>(),
            .portainer_id = stack["portainer_id"].as<int>(0),
        };
        if (stack["containers"]) {
            std::vector<Container> containers;
            for (const auto& container : stack["containers"]) {
                containers.push_back(Container{
                    .slug = container["slug"].as<std::string>(),
                    .name = container["name"].as<std::string>(),
                    .icon = container["icon"].as<std::string>()});
            }
            stackObj.containers = containers;
        }
        stacks.push_back(stackObj);
    }
    return stacks;
}