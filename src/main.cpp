#include <iostream>
#include <cstdlib>
#include <memory>

#include "ha/ha_client.h"
#include "ws/boost_ws_connection.h"
#include "dashboard/dashboard_updater.h"
#include "config/config_parser.h"

void run(const std::string& url, const std::string& token, const std::string& portainer_url) {
    auto ws = std::make_unique<BoostWebSocketConnection>();
    HomeAssistantClient client(url, token, std::move(ws));

    std::cout << "Connecting to Home Assistant..." << std::endl;
    client.connect();

    std::cout << "Fetching dashboard config..." << std::endl;
    auto config = client.getDashboardConfig("dashboard-test");

    std::cout << "Updating Portainer view..." << std::endl;
    auto stacks = loadStacks("/app/stacks.yaml");
    DashboardUpdater updater(portainer_url, stacks);
    auto updated = updater.updateDashboard(config);

    std::cout << "Saving dashboard..." << std::endl;
    client.saveDashboardConfig("dashboard-test", updated);

    std::cout << "Done!" << std::endl;
    client.disconnect();
}

int main() {
    const char* url = std::getenv("HA_URL");
    const char* token = std::getenv("HA_TOKEN");
    const char* portainer_url = std::getenv("PORTAINER_URL");

    if (!url || !token || !portainer_url) {
        std::cerr << "Error: HA_URL, HA_TOKEN, and PORTAINER_URL environment variables must be set" << std::endl;
        return 1;
    }

    try {
        run(url, token, portainer_url);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}