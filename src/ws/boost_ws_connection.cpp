#include "boost_ws_connection.h"
#include "../errors.h"
#include <openssl/ssl.h>
#include <iostream>

void BoostWebSocketConnection::connect(const std::string& host,
                                       const std::string& port,
                                       const std::string& path) {
    try {
        // DNS resolution
        net::ip::tcp::resolver resolver(ioc_);
        auto results = resolver.resolve(host, port);

        // Create the WebSocket stream object
        ws_ = std::make_unique<ws_stream>(ioc_, ssl_ctx_);

        // TCP connect
        net::connect(ws_->next_layer().next_layer(), results);

        // Set SNI for TLS
        SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host.c_str());

        // SSL handshake
        ws_->next_layer().handshake(ssl::stream_base::client);

        // WebSocket handshake
        ws_->handshake(host, path);
    } catch (const boost::system::system_error& e) {
        throw ConnectionError("Could not connect to " + host + ":" + port
                              + ": " + e.what());
    }
}

std::string BoostWebSocketConnection::read() {
    try {
        beast::flat_buffer buffer;
        ws_->read(buffer);
        return beast::buffers_to_string(buffer.data());
    } catch (const boost::system::system_error& e) {
        throw ConnectionError(std::string("WebSocket read failed: ") + e.what());
    }
}

void BoostWebSocketConnection::write(const std::string& message) {
    try {
        ws_->write(net::buffer(message));
    } catch (const boost::system::system_error& e) {
        throw ConnectionError(std::string("WebSocket write failed: ") + e.what());
    }
}

void BoostWebSocketConnection::close() {
    if (ws_) {
        try {
            ws_->close(websocket::close_code::normal);
            ws_->next_layer().shutdown();
        } catch (const std::exception& e) {
            std::cerr << "Warning: error during WebSocket close: " << e.what() << std::endl;
        }
        ws_.reset();
    }
}
