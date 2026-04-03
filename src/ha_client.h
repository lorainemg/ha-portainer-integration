#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

using json = nlohmann::json;

// Type aliases to avoid typing these long namespaces everywhere.
// In C++ nested namespaces get verbose fast — aliasing is standard practice.
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

// This is the full type of our WebSocket connection:
// websocket::stream wraps ssl::stream wraps tcp socket.
// Each layer adds a protocol on top of the previous one.
using ws_stream = websocket::stream<ssl::stream<net::ip::tcp::socket>>;

class HomeAssistantClient {
public:
    HomeAssistantClient(const std::string& url, const std::string& token);

    void connect();
    void disconnect();

    json listDashboards();
    json getDashboardConfig(const std::string& url_path);
    void saveDashboardConfig(const std::string& url_path, const json& config);

    // Grant test class access to private members
    friend class HomeAssistantClientTest;

private:
    std::string host_;
    std::string port_;
    std::string path_;
    std::string token_;
    int next_id_ = 1;

    // The three layers of our connection:
    net::io_context ioc_;                  // Event loop
    ssl::context ssl_ctx_{ssl::context::tls_client};  // SSL config
    std::unique_ptr<ws_stream> ws_;        // WebSocket (created on connect)

    json sendCommand(const json& message);
};
