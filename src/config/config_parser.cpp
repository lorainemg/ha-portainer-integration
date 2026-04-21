#include "config_parser.h"
#include "../errors.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

// Helper to check a required field exists and return its value
static std::string requireField(const YAML::Node& node,
                                const std::string& field,
                                const std::string& context) {
    if (!node[field]) {
        throw ConfigError(context + " missing required field '" + field + "'");
    }
    return node[field].as<std::string>();
}

// Reads an optional YAML sequence of strings; returns empty vector if absent.
static std::vector<std::string> readStringList(const YAML::Node& node) {
    std::vector<std::string> out;
    if (!node || !node.IsSequence()) return out;
    for (size_t i = 0; i < node.size(); ++i) {
        out.push_back(node[i].as<std::string>());
    }
    return out;
}

YamlState loadYamlState(const std::string& filepath) {
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

    YamlState state;
    for (size_t i = 0; i < yamlData["stacks"].size(); ++i) {
        const auto& stack = yamlData["stacks"][i];
        std::string ctx = "Stack #" + std::to_string(i + 1);

        Stack stackObj{
            .slug = requireField(stack, "slug", ctx),
            .name = requireField(stack, "name", ctx),
            .icon = requireField(stack, "icon", ctx),
            .portainer_id = stack["portainer_id"].as<int>(0),
        };

        if (stack["containers"]) {
            for (size_t j = 0; j < stack["containers"].size(); ++j) {
                const auto& container = stack["containers"][j];
                std::string cctx = ctx + ", container #" + std::to_string(j + 1);
                stackObj.containers.push_back(Container{
                    .slug = requireField(container, "slug", cctx),
                    .name = requireField(container, "name", cctx),
                    .icon = requireField(container, "icon", cctx)});
            }
        }
        stackObj.ignored_containers = readStringList(stack["ignored_containers"]);
        state.stacks.push_back(stackObj);
    }
    state.ignored = readStringList(yamlData["ignored"]);
    return state;
}

void saveYamlState(const std::string& filepath, const YamlState& state) {
    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "stacks" << YAML::Value << YAML::BeginSeq;
    for (const auto& stack : state.stacks) {
        out << YAML::BeginMap;
        out << YAML::Key << "slug" << YAML::Value << stack.slug;
        out << YAML::Key << "name" << YAML::Value << stack.name;
        out << YAML::Key << "icon" << YAML::Value << stack.icon;
        out << YAML::Key << "portainer_id" << YAML::Value << stack.portainer_id;

        if (!stack.containers.empty()) {
            out << YAML::Key << "containers" << YAML::Value << YAML::BeginSeq;
            for (const auto& c : stack.containers) {
                out << YAML::BeginMap;
                out << YAML::Key << "slug" << YAML::Value << c.slug;
                out << YAML::Key << "name" << YAML::Value << c.name;
                out << YAML::Key << "icon" << YAML::Value << c.icon;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
        if (!stack.ignored_containers.empty()) {
            out << YAML::Key << "ignored_containers" << YAML::Value << YAML::BeginSeq;
            for (const auto& slug : stack.ignored_containers) out << slug;
            out << YAML::EndSeq;
        }
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    if (!state.ignored.empty()) {
        out << YAML::Key << "ignored" << YAML::Value << YAML::BeginSeq;
        for (const auto& slug : state.ignored) out << slug;
        out << YAML::EndSeq;
    }

    out << YAML::EndMap;

    std::ofstream f(filepath);
    if (!f) throw ConfigError("Could not open config file for writing: " + filepath);
    f << out.c_str();
}
