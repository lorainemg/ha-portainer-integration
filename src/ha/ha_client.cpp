#include "ha_client.h"
#include "../errors.h"
#include <iostream>

HomeAssistantClient::HomeAssistantClient(const std::string& url,
                                         const std::string& token,
                                         std::unique_ptr<IWebSocketConnection> connection)
    : token_(token), ws_(std::move(connection)) {

    // Validate URL format: must have "://" and a path after the host
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        throw ConfigError("Invalid WebSocket URL (missing ://): " + url);
    }

    auto host_start = scheme_end + 3;
    auto path_start = url.find('/', host_start);
    if (path_start == std::string::npos) {
        throw ConfigError("Invalid WebSocket URL (missing path): " + url);
    }

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
    ws_->connect(host_, port_, path_);

    try {
        auto auth_required = json::parse(ws_->read());
    } catch (const json::parse_error& e) {
        throw AuthError(std::string("Failed to parse auth_required message: ") + e.what());
    }

    json auth_msg = {{"type", "auth"}, {"access_token", token_}};
    ws_->write(auth_msg.dump());

    json response;
    try {
        response = json::parse(ws_->read());
    } catch (const json::parse_error& e) {
        throw AuthError(std::string("Failed to parse auth response: ") + e.what());
    }

    if (response["type"] != "auth_ok") {
        std::string detail = response.value("message", response.dump());
        throw AuthError("Authentication failed: " + detail);
    }
}

void HomeAssistantClient::disconnect() const {
    if (ws_) {
        try {
            ws_->close();
        } catch (const std::exception& e) {
            std::cerr << "Warning: error during disconnect: " << e.what() << std::endl;
        }
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

    json response;
    try {
        response = json::parse(ws_->read());
    } catch (const json::parse_error& e) {
        throw CommandError(std::string("Failed to parse HA response: ") + e.what());
    }

    if (!response.value("success", false)) {
        std::string detail;
        if (response.contains("error") && response["error"].contains("message")) {
            detail = response["error"]["message"].get<std::string>();
        } else {
            detail = response.dump();
        }
        throw CommandError("Command failed: " + detail);
    }

    return response["result"];
}
