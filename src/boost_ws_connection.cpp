#include "boost_ws_connection.h"

void BoostWebSocketConnection::connect(const std::string& host,
                                       const std::string& port,
                                       const std::string& path) {
    // DNS resolution: turn hostname into IP address(es)
    net::ip::tcp::resolver resolver(ioc_);
    auto results = resolver.resolve(host, port);

    // Create the WebSocket stream object (WebSocket → SSL → TCP)
    ws_ = std::make_unique<ws_stream>(ioc_, ssl_ctx_);

    // TCP connect: open a raw socket to the resolved IP
    net::connect(ws_->next_layer().next_layer(), results);

    // SSL handshake: negotiate encryption over the TCP connection
    ws_->next_layer().handshake(ssl::stream_base::client);

    // WebSocket handshake: send HTTP Upgrade request to switch to WebSocket
    ws_->handshake(host, path);
}

std::string BoostWebSocketConnection::read() {
    beast::flat_buffer buffer;
    ws_->read(buffer);
    return beast::buffers_to_string(buffer.data());
}

void BoostWebSocketConnection::write(const std::string& message) {
    ws_->write(net::buffer(message));
}

void BoostWebSocketConnection::close() {
    if (ws_) {
        ws_->close(websocket::close_code::normal);
        ws_->next_layer().shutdown();
        ws_.reset();
    }
}