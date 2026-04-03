#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "i_ws_connection.h"

using json = nlohmann::json;

class HomeAssistantClient {
public:
    // Constructor now receives a connection from outside (dependency injection).
    // std::unique_ptr<IWebSocketConnection> = a smart pointer to the interface.
    // std::move() transfers ownership — after the caller moves it in,
    // they no longer own it; the client does.
    HomeAssistantClient(const std::string& url,
                        const std::string& token,
                        std::unique_ptr<IWebSocketConnection> connection);

    void connect();
    void disconnect();

    json listDashboards();
    json getDashboardConfig(const std::string& url_path);
    void saveDashboardConfig(const std::string& url_path, const json& config);

    friend class HomeAssistantClientTest;

private:
    std::string host_;
    std::string port_;
    std::string path_;
    std::string token_;
    int next_id_ = 1;

    // The connection is now an interface pointer — could be
    // BoostWebSocketConnection (real) or a mock (tests)
    std::unique_ptr<IWebSocketConnection> ws_;

    json sendCommand(const json& message);
};
