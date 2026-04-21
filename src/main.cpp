#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>

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

void run(const std::string& ha_url,
         const std::string& ha_token,
         const std::string& portainer_url,
         const std::string& portainer_token,
         int endpoint_id) {
    const std::string yaml_path = "/app/stacks.yaml";
    auto state = loadYamlState(yaml_path);

    std::cout << "Querying Portainer..." << std::endl;
    PortainerClient portainer(portainer_url, portainer_token, endpoint_id,
                              std::make_unique<BoostHttpClient>());
    auto p_stacks     = portainer.listStacks();
    auto p_containers = portainer.listAllContainers();

    auto diff = reconcile(state, p_stacks, p_containers);

    if (diff.needsPrompts()) {
        auto decisions = promptForDiff(diff);
        if (decisions.quit) {
            std::cout << "Aborted by user. No changes written." << std::endl;
            return;
        }
        applyDecisions(state, diff, decisions, p_stacks);
        saveYamlState(yaml_path, state);
        std::cout << "Saved updated " << yaml_path << std::endl;
    } else if (!diff.id_changes.empty()) {
        applyDecisions(state, diff, {}, p_stacks);
        saveYamlState(yaml_path, state);
        std::cout << "Applied " << diff.id_changes.size() << " silent id update(s)." << std::endl;
    } else {
        std::cout << "No changes vs. Portainer." << std::endl;
    }

    auto ws = std::make_unique<BoostWebSocketConnection>();
    HomeAssistantClient client(ha_url, ha_token, std::move(ws));
    std::cout << "Connecting to Home Assistant..." << std::endl;
    client.connect();
    auto config = client.getDashboardConfig("dashboard-test");
    DashboardUpdater updater(portainer_url, state.stacks);
    auto updated = updater.updateDashboard(config);
    client.saveDashboardConfig("dashboard-test", updated);
    client.disconnect();
    std::cout << "Done!" << std::endl;
}

int main() {
    const char* ha_url           = std::getenv("HA_URL");
    const char* ha_token         = std::getenv("HA_TOKEN");
    const char* portainer_url    = std::getenv("PORTAINER_URL");
    const char* portainer_token  = std::getenv("PORTAINER_TOKEN");
    const char* endpoint_str     = std::getenv("PORTAINER_ENDPOINT_ID");

    if (!ha_url || !ha_token || !portainer_url || !portainer_token) {
        std::cerr << "Error: HA_URL, HA_TOKEN, PORTAINER_URL, and PORTAINER_TOKEN must be set"
                  << std::endl;
        return 1;
    }
    int endpoint_id = endpoint_str ? std::atoi(endpoint_str) : 3;

    try {
        run(ha_url, ha_token, portainer_url, portainer_token, endpoint_id);
    } catch (const HaPortainerError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
