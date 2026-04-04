#include "dashboard_updater.h"
#include <algorithm>  // for std::find_if
#include <utility>    // for std::move

DashboardUpdater::DashboardUpdater(std::string portainer_base,
                                   std::vector<Stack> stacks)
    : portainer_base_(std::move(portainer_base)), stacks_(std::move(stacks)) {}

std::string DashboardUpdater::ht(const std::string& expr) {
    return "{{ " + expr + " }}";
}

json DashboardUpdater::generateView() const {
    // Build sections: docker host first, then each stack
    json sections = json::array();
    sections.push_back(dockerHostSection());

    for (const auto& s : stacks_) {
        if (s.isExpander()) {
            sections.push_back(sectionExpander(s));
        } else {
            sections.push_back(sectionStandalone(s));
        }
    }

    return {
        {"type", "sections"},
        {"max_columns", 4},
        {"title", "Portainer"},
        {"path", "test-portainer"},
        {"icon", "mdi:docker"},
        {"subview", false},
        {"badges", buildBadges()},
        {"sections", sections},
        {"cards", json::array()}
    };
}

json DashboardUpdater::updateDashboard(const json& existing_config) const {
    // Copy the config so we don't modify the original
    json config = existing_config;
    json& views = config["views"];

    json new_view = generateView();

    // Search for an existing Portainer view by path
    // std::find_if walks the array and returns an iterator to the first match
    auto it = std::find_if(views.begin(), views.end(), [](const json& view) {
        return view.contains("path") && view["path"] == "test-portainer";
    });

    if (it != views.end()) {
        // Found it — replace in place
        // *it dereferences the iterator to get the actual element
        *it = new_view;
    } else {
        // Not found — append as a new view
        views.push_back(new_view);
    }

    return config;
}

json DashboardUpdater::dockerHostSection() const {
    std::string status = "binary_sensor.local_status";

    return {
        {"type", "grid"},
        {"column_span", 4},
        {"cards", {
            {
                {"type", "custom:stack-in-card"},
                {"title", "Docker Host"},
                {"mode", "vertical"},
                {"grid_options", {{"columns", "full"}}},
                {"cards", {
                    // Host header card
                    {
                        {"type", "custom:mushroom-template-card"},
                        {"primary", "Docker Host"},
                        {"secondary", ht("states('sensor.local_operating_system')") + " "
                                    + ht("states('sensor.local_operating_system_version')") + " · Kernel "
                                    + ht("states('sensor.local_kernel_version')") + " · Docker "
                                    + ht("states('sensor.local_docker_version')")},
                        {"icon", "mdi:server"},
                        {"icon_color", ht("'green' if is_state('" + status + "', 'on') else 'red'")},
                        {"badge_icon", ht("'mdi:check-circle' if is_state('" + status + "', 'on') else 'mdi:alert-circle'")},
                        {"badge_color", ht("'green' if is_state('" + status + "', 'on') else 'red'")},
                        {"tap_action", {{"action", "url"}, {"url_path", portainer_base_ + "/#!/3/docker/dashboard"}}}
                    },
                    // Container counts row
                    {
                        {"type", "horizontal-stack"},
                        {"cards", {
                            entityCard("sensor.local_container_count", "Containers", "mdi:docker", "blue"),
                            entityCard("sensor.local_containers_running", "Running", "mdi:play-circle-outline", "green"),
                            {
                                {"type", "custom:mushroom-entity-card"},
                                {"entity", "sensor.local_containers_stopped"},
                                {"name", "Stopped"},
                                {"icon", "mdi:stop-circle-outline"},
                                {"icon_color", ht("'red' if states('sensor.local_containers_stopped') | int > 0 else 'grey'")}
                            }
                        }}
                    },
                    // Resources row
                    {
                        {"type", "horizontal-stack"},
                        {"cards", {
                            entityCard("sensor.local_image_count", "Images", "mdi:layers-outline", "purple"),
                            entityCard("sensor.local_total_cpu", "CPU", "mdi:cpu-64-bit", "orange"),
                            entityCard("sensor.local_total_memory", "Memory", "mdi:memory", "cyan")
                        }}
                    },
                    // Prune button
                    {
                        {"type", "custom:mushroom-entity-card"},
                        {"entity", "button.local_prune_unused_images"},
                        {"name", "Prune Unused Images"},
                        {"icon", "mdi:delete-sweep-outline"},
                        {"icon_color", "red"}
                    }
                }}
            }
        }}
    };
}

json DashboardUpdater::sectionStandalone(const Stack& stack) const {
    return {
        {"type", "grid"},
        {"column_span", 2},
        {"cards", {
            {
                {"type", "custom:stack-in-card"},
                {"title", stack.name},
                {"grid_options", {{"columns", "full"}}},
                {"mode", "vertical"},
                {"cards", {
                    mushroomHeader(stack.slug, stack.name, stack.icon,
                                   stack.status(), stack.portainerUrl(portainer_base_)),
                    statsRow(stack.slug),
                    controlsRow(stack.slug)
                }}
            }
        }}
    };
}

json DashboardUpdater::sectionExpander(const Stack& stack) const {
    // Title card for the expander header
    json title_card = {
        {"type", "custom:mushroom-template-card"},
        {"primary", stack.name},
        {"secondary", "Stack · " + ht("states('sensor." + stack.slug + "_type') | capitalize")
                     + " · " + ht("states('sensor." + stack.slug + "_containers')") + " container(s)"},
        {"icon", stack.icon},
        {"icon_color", ht("'green' if is_state('" + stack.status() + "', 'on') else 'red'")},
        {"badge_icon", ht("'mdi:check-circle' if is_state('" + stack.status() + "', 'on') else 'mdi:alert-circle'")},
        {"badge_color", ht("'green' if is_state('" + stack.status() + "', 'on') else 'red'")},
        {"tap_action", {{"action", "url"}, {"url_path", stack.portainerUrl(portainer_base_)}}}
    };

    // Build child cards: stack power switch + a stack-in-card for each container
    json child_cards = json::array();

    child_cards.push_back({
        {"type", "custom:mushroom-entity-card"},
        {"entity", stack.stackSwitch()},
        {"name", "Stack Power"},
        {"icon_color", "green"}
    });

    // Loop over each container in the stack
    for (const auto& c : stack.containers) {
        child_cards.push_back({
            {"type", "custom:stack-in-card"},
            {"title", c.name},
            {"mode", "vertical"},
            {"cards", {
                mushroomHeader(c.slug, c.name, c.icon,
                               "binary_sensor." + c.slug + "_status", "", false),
                statsRow(c.slug),
                controlsRow(c.slug, stack.stackSwitch())
            }}
        });
    }

    return {
        {"type", "grid"},
        {"column_span", 2},
        {"cards", {
            {
                {"type", "custom:expander-card"},
                {"expanded", false},
                {"title", stack.name},
                {"title-card", title_card},
                {"cards", child_cards},
                {"grid_options", {{"columns", "full"}}}
            }
        }}
    };
}

json DashboardUpdater::buildBadges() const {
    std::string status = "binary_sensor.local_status";

    json badges = {
        {
            {"type", "custom:mushroom-template-badge"},
            {"content", "Host"},
            {"icon", "mdi:server-network"},
            {"color", ht("'green' if is_state('" + status + "', 'on') else 'red'")}
        },
        {
            {"type", "custom:mushroom-template-badge"},
            {"content", ht("states('sensor.local_containers_running') + ' running'")},
            {"icon", "mdi:play-circle"},
            {"color", "green"}
        },
        {
            {"type", "custom:mushroom-template-badge"},
            {"content", ht("states('sensor.local_containers_stopped') + ' stopped'")},
            {"icon", "mdi:stop-circle"},
            {"color", ht("'red' if states('sensor.local_containers_stopped') | int > 0 else 'disabled'")}
        }
    };

    for (const auto& s : stacks_) {
        badges.push_back({
            {"type", "entity"},
            {"entity", s.status()},
            {"name", s.name},
            {"show_name", true}
        });
    }

    return badges;
}

json DashboardUpdater::entityCard(const std::string& entity, const std::string& name,
                                  const std::string& icon, const std::string& icon_color) {
    return {
        {"type", "custom:mushroom-entity-card"},
        {"entity", entity},
        {"name", name},
        {"icon", icon},
        {"icon_color", icon_color}
    };
}

json DashboardUpdater::statsRow(const std::string& slug) {
    return {
        {"type", "horizontal-stack"},
        {"cards", {
            entityCard("sensor." + slug + "_memory_usage_percentage", "Mem %", "mdi:memory", "cyan"),
            entityCard("sensor." + slug + "_memory_usage", "Memory", "mdi:chip", "teal"),
            entityCard("sensor." + slug + "_cpu_usage_total", "CPU", "mdi:cpu-64-bit", "orange")
        }}
    };
}

json DashboardUpdater::controlsRow(const std::string& slug, const std::string& stack_switch) {
    // json::array() creates an empty JSON array — like [] in Python
    // We build it up with .push_back(), like Python's .append()
    json cards = json::array();

    if (!stack_switch.empty()) {
        cards.push_back({
            {"type", "custom:mushroom-entity-card"},
            {"entity", stack_switch},
            {"name", "Stack"},
            {"icon_color", "green"}
        });
    }

    cards.push_back({
        {"type", "custom:mushroom-entity-card"},
        {"entity", "switch." + slug + "_container"},
        {"name", stack_switch.empty() ? "Power" : "Container"},
        {"icon_color", "green"}
    });

    cards.push_back({
        {"type", "custom:mushroom-entity-card"},
        {"entity", "button." + slug + "_restart_container"},
        {"name", "Restart"},
        {"icon_color", "orange"}
    });

    return {{"type", "horizontal-stack"}, {"cards", cards}};
}

json DashboardUpdater::mushroomHeader(const std::string& slug, const std::string& name,
                                      const std::string& icon, const std::string& status,
                                      const std::string& url, bool badge) {
    auto green_red = ht("'green' if is_state('" + status + "', 'on') else 'red'");

    json card = {
        {"type", "custom:mushroom-template-card"},
        {"primary", name},
        {"secondary", ht("states('sensor." + slug + "_state') | capitalize") + " · "
                    + ht("states('sensor." + slug + "_image').split('/') | last")},
        {"icon", icon},
        {"icon_color", green_red}
    };

    if (badge) {
        card["badge_icon"] = ht("'mdi:check-circle' if is_state('" + status + "', 'on') else 'mdi:alert-circle'");
        card["badge_color"] = green_red;
    }

    if (!url.empty()) {
        card["tap_action"] = {{"action", "url"}, {"url_path", url}};
    }

    return card;
}