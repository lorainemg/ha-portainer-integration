#pragma once

#include "i_ws_connection.h"

#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

using ws_stream = websocket::stream<ssl::stream<net::ip::tcp::socket>>;

class BoostWebSocketConnection : public IWebSocketConnection {
public:
    // override: tells the compiler "I'm implementing a virtual method"
    void connect(const std::string& host,
                 const std::string& port,
                 const std::string& path) override;

    std::string read() override;

    void write(const std::string& message) override;

    void close() override;

private:
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tls_client};
    std::unique_ptr<ws_stream> ws_;
};
