#include "ha_client.h"

HomeAssistantClient::HomeAssistantClient(const std::string& url, const std::string& token)
    : token_(token) {
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
    // DNS resolution: turn hostname into IP address(es)
    net::ip::tcp::resolver resolver(ioc_);
    auto results = resolver.resolve(host_, port_);

    // Create the WebSocket stream object (WebSocket → SSL → TCP)
    // make_unique creates it on the heap, owned by the unique_ptr ws_
    ws_ = std::make_unique<ws_stream>(ioc_, ssl_ctx_);

    // TCP connect: open a raw socket to the resolved IP
    // next_layer() peels back one layer: ws(websocket) → ssl → tcp
    // so next_layer().next_layer() gets the TCP socket
    net::connect(ws_->next_layer().next_layer(), results);

    // SSL handshake: negotiate encryption over the TCP connection
    ws_->next_layer().handshake(ssl::stream_base::client);

    // WebSocket handshake: send HTTP Upgrade request to switch to WebSocket
    ws_->handshake(host_, path_);

    // --- HA authentication ---

    // Buffer to hold incoming data (like a bytearray that Beast writes into)
    beast::flat_buffer buffer;

    // Read the auth_required message from HA (blocks until received)
    ws_->read(buffer);
    buffer.clear();

    // Build auth JSON: {{"key", value}} is nlohmann's initializer syntax
    // .dump() converts to string, like json.dumps() in Python
    // net::buffer() wraps the string so Beast can read from it
    json auth_msg = {{"type", "auth"}, {"access_token", token_}};
    ws_->write(net::buffer(auth_msg.dump()));

    // Read the auth response
    ws_->read(buffer);
    // buffers_to_string converts raw bytes to std::string, then json::parse
    // parses it — like json.loads() in Python
    auto response = json::parse(beast::buffers_to_string(buffer.data()));

    if (response["type"] != "auth_ok") {
        throw std::runtime_error("Authentication failed");
    }
}

void HomeAssistantClient::disconnect() {
    if (ws_) {
        // WebSocket close: sends a close frame to the server
        ws_->close(websocket::close_code::normal);

        // SSL shutdown: tear down the encrypted channel
        ws_->next_layer().shutdown();

        // Reset the pointer to nullptr (frees the stream object)
        ws_.reset();
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
    // Copy the message so we can add the id without modifying the original
    json msg = message;
    msg["id"] = next_id_++;  // use current id, then increment

    // Send and read response (same pattern as the auth handshake)
    ws_->write(net::buffer(msg.dump()));

    beast::flat_buffer buffer;
    ws_->read(buffer);
    auto response = json::parse(beast::buffers_to_string(buffer.data()));

    if (!response.value("success", false)) {
        throw std::runtime_error("Command failed: " + response.dump());
    }

    return response["result"];
}
