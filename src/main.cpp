#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "errors.h"
#include "ha/ha_client.h"
#include "ws/boost_ws_connection.h"
#include "dashboard/dashboard_updater.h"
#include "config/config_parser.h"
#include "portainer/boost_http_client.h"
#include "portainer/portainer_client.h"
#include "discovery/reconciler.h"
#include "discovery/prompter.h"
#include "discovery/apply.h"

// Everything one full run of the tool needs, gathered in one place.
struct Config {
    std::string ha_url;
    std::string ha_token;
    std::string portainer_url;
    std::string portainer_token;
    int endpoint_id;
    bool auto_accept;
};

struct PortainerSnapshot {
    std::vector<PortainerStack> stacks;
    std::vector<PortainerContainer> containers;
};

// Hits Portainer's HTTP API and returns everything the reconciler needs.
static PortainerSnapshot fetchFromPortainer(const Config& cfg) {
    std::cout << "Querying Portainer..." << std::endl;
    PortainerClient portainer(cfg.portainer_url, cfg.portainer_token,
                              cfg.endpoint_id, std::make_unique<BoostHttpClient>());
    return { portainer.listStacks(), portainer.listAllContainers() };
}

// Produces Decisions for a diff, either by prompting the user or by auto-accepting.
static Decisions collectDecisions(const DiffReport& diff, bool auto_accept) {
    if (auto_accept) {
        std::cout << "Auto-accepting "
                  << diff.new_stacks.size() << " new stack(s), "
                  << diff.gone_stacks.size() << " gone stack(s), "
                  << diff.new_containers.size() << " new container(s), "
                  << diff.gone_containers.size() << " gone container(s)." << std::endl;
        return allYes(diff);
    }
    return promptForDiff(diff);
}

// Connects to HA via WebSocket, regenerates the Portainer view, disconnects.
static void pushDashboardToHA(const Config& cfg, const YamlState& state) {
    auto ws = std::make_unique<BoostWebSocketConnection>();
    HomeAssistantClient client(cfg.ha_url, cfg.ha_token, std::move(ws));
    std::cout << "Connecting to Home Assistant..." << std::endl;
    client.connect();
    auto dash = client.getDashboardConfig("dashboard-test");
    DashboardUpdater updater(cfg.portainer_url, state.stacks);
    auto updated = updater.updateDashboard(dash);
    client.saveDashboardConfig("dashboard-test", updated);
    client.disconnect();
    std::cout << "Done!" << std::endl;
}

// Top-level orchestration: load yaml, reconcile vs Portainer, save, push to HA.
static void run(const Config& cfg) {
    const std::string yaml_path = "/app/stacks.yaml";
    auto state = loadYamlState(yaml_path);
    auto snap  = fetchFromPortainer(cfg);
    auto diff  = reconcile(state, snap.stacks, snap.containers);

    if (diff.needsPrompts()) {
        auto decisions = collectDecisions(diff, cfg.auto_accept);
        if (decisions.quit) {
            std::cout << "Aborted by user. No changes written." << std::endl;
            return;
        }
        applyDecisions(state, diff, decisions, snap.stacks);
        saveYamlState(yaml_path, state);
        std::cout << "Saved updated " << yaml_path << std::endl;
    } else if (!diff.id_changes.empty()) {
        applyDecisions(state, diff, {}, snap.stacks);
        saveYamlState(yaml_path, state);
        std::cout << "Applied " << diff.id_changes.size() << " silent id update(s)." << std::endl;
    } else {
        std::cout << "No changes vs. Portainer." << std::endl;
    }

    pushDashboardToHA(cfg, state);
}

int main(int argc, char** argv) {
    Config cfg{};
    cfg.auto_accept = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--yes" || arg == "-y") {
            cfg.auto_accept = true;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            std::cerr << "Usage: ha-portainer [--yes | -y]" << std::endl;
            return 2;
        }
    }

    const char* ha_url          = std::getenv("HA_URL");
    const char* ha_token        = std::getenv("HA_TOKEN");
    const char* portainer_url   = std::getenv("PORTAINER_URL");
    const char* portainer_token = std::getenv("PORTAINER_TOKEN");
    const char* endpoint_str    = std::getenv("PORTAINER_ENDPOINT_ID");

    if (!ha_url || !ha_token || !portainer_url || !portainer_token) {
        std::cerr << "Error: HA_URL, HA_TOKEN, PORTAINER_URL, and PORTAINER_TOKEN must be set"
                  << std::endl;
        return 1;
    }
    cfg.ha_url          = ha_url;
    cfg.ha_token        = ha_token;
    cfg.portainer_url   = portainer_url;
    cfg.portainer_token = portainer_token;
    // Treat empty or unset PORTAINER_ENDPOINT_ID as "use default" (3).
    cfg.endpoint_id     = (endpoint_str && endpoint_str[0] != '\0')
                          ? std::atoi(endpoint_str) : 3;

    try {
        run(cfg);
    } catch (const HaPortainerError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}