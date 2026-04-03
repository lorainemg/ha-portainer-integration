#include <iostream>
#include <cstdlib>
#include <memory>

#include "ha/ha_client.h"
#include "ws/boost_ws_connection.h"
#include "dashboard/dashboard_updater.h"
#include "dashboard/stack.h"

std::vector<Stack> buildStacks() {
    return {
        {"netdata",   "Netdata",   "mdi:chart-line",    10, {}},
        {"portainer", "Portainer", "mdi:anchor",        0,  {}},
        {"caddy",     "Caddy",     "mdi:image-multiple", 9, {
            {"caddy",       "Caddy",       "mdi:shield-check"},
            {"cloudflared", "Cloudflared", "mdi:cloud-arrow-up"},
        }},
        {"home_assistant", "Home Assistant", "mdi:image-multiple", 13, {
            {"homeassistant", "Home Assistant", "mdi:home-assistant"},
            {"mosquitto",     "Mosquitto",      "mdi:message-arrow-up-down"},
        }},
        {"immich", "Immich", "mdi:image-multiple", 1, {
            {"immich_server",           "Server",           "mdi:server"},
            {"immich_machine_learning", "Machine Learning", "mdi:brain"},
            {"immich_postgres",         "Postgres",         "mdi:database"},
            {"immich_redis",            "Redis",            "mdi:database-arrow-up"},
        }},
    };
}

void run(const std::string& url, const std::string& token) {
    auto ws = std::make_unique<BoostWebSocketConnection>();
    HomeAssistantClient client(url, token, std::move(ws));

    std::cout << "Connecting to Home Assistant..." << std::endl;
    client.connect();

    std::cout << "Fetching dashboard config..." << std::endl;
    auto config = client.getDashboardConfig("dashboard-test");

    std::cout << "Updating Portainer view..." << std::endl;
    DashboardUpdater updater("https://portainer.sussman.win", buildStacks());
    auto updated = updater.updateDashboard(config);

    std::cout << "Saving dashboard..." << std::endl;
    client.saveDashboardConfig("dashboard-test", updated);

    std::cout << "Done!" << std::endl;
    client.disconnect();
}

int main() {
    const char* url = std::getenv("HA_URL");
    const char* token = std::getenv("HA_TOKEN");

    if (!url || !token) {
        std::cerr << "Error: HA_URL and HA_TOKEN environment variables must be set" << std::endl;
        return 1;
    }

    try {
        run(url, token);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}