#include "ha_client.h"

// Constructor now takes a connection via std::move (transfers ownership)
HomeAssistantClient::HomeAssistantClient(const std::string& url,
                                         const std::string& token,
                                         std::unique_ptr<IWebSocketConnection> connection)
    : token_(token), ws_(std::move(connection)) {
    // url = "wss://homeassistant.sussman.win/api/websocket"

    auto host_start = url.find("://") + 3;        // position after "wss://"
    auto path_start = url.find('/', host_start);   // position of "/api/websocket"

    auto host_and_port = url.substr(host_start, path_start - host_start);
    path_ = url.substr(path_start);

    auto colon = host_and_port.find(':');
    if (colon != std::string::npos) {
        host_ = host_and_port.substr(0, colon);
        port_ = host_and_port.substr(colon + 1);
    } else {
        host_ = host_and_port;
        port_ = "443";
    }
}

void HomeAssistantClient::connect() {
    // Now just delegates to the interface — all Boost details are hidden
    ws_->connect(host_, port_, path_);

    // --- HA authentication ---

    // Read the auth_required message from HA
    ws_->read();

    // Send auth token as JSON
    json auth_msg = {{"type", "auth"}, {"access_token", token_}};
    ws_->write(auth_msg.dump());

    // Read the auth response and check it
    auto response = json::parse(ws_->read());

    if (response["type"] != "auth_ok") {
        throw std::runtime_error("Authentication failed");
    }
}

void HomeAssistantClient::disconnect() const {
    if (ws_) {
        ws_->close();
    }
}

json HomeAssistantClient::listDashboards() {
    return sendCommand({{"type", "lovelace/dashboards/list"}});
}

json HomeAssistantClient::getDashboardConfig(const std::string& url_path) {
    json cmd = {{"type", "lovelace/config"}};
    if (!url_path.empty()) {
        cmd["url_path"] = url_path;
    }
    return sendCommand(cmd);
}

void HomeAssistantClient::saveDashboardConfig(const std::string& url_path, const json& config) {
    sendCommand({
        {"type", "lovelace/config/save"},
        {"url_path", url_path},
        {"config", config}
    });
}

json HomeAssistantClient::sendCommand(const json& message) {
    json msg = message;
    msg["id"] = next_id_++;

    ws_->write(msg.dump());

    auto response = json::parse(ws_->read());

    if (!response.value("success", false)) {
        throw std::runtime_error("Command failed: " + response.dump());
    }

    return response["result"];
}