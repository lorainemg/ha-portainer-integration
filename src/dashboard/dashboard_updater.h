#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "stack.h"

using json = nlohmann::json;

class DashboardUpdater {
public:
    DashboardUpdater(std::string portainer_base,
                     std::vector<Stack> stacks);

    [[nodiscard]] json generateView() const;

    [[nodiscard]] json updateDashboard(const json& existing_config) const;

private:
    std::string portainer_base_;
    std::vector<Stack> stacks_;

    // Card builders
    [[nodiscard]] json buildHeader() const;
    [[nodiscard]] json dockerHostSection() const;
    [[nodiscard]] json sectionStandalone(const Stack& stack) const;
    [[nodiscard]] json sectionExpander(const Stack& stack) const;
    [[nodiscard]] json buildBadges() const;

    // Wraps an expression in {{ }} for HA Jinja2 templates
    static std::string ht(const std::string& expr);

    // Reusable card helpers — static because they don't use member variables
    static json entityCard(const std::string& entity, const std::string& name,
                           const std::string& icon, const std::string& icon_color);
    static json statsRow(const std::string& slug);
    static json controlsRow(const std::string& slug, const std::string& stack_switch = "");
    static json mushroomHeader(const std::string& slug, const std::string& name,
                               const std::string& icon, const std::string& status,
                               const std::string& url = "", bool badge = true);
};