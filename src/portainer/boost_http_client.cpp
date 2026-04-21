#include "boost_http_client.h"
#include "../errors.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
namespace ssl   = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// Splits "https://host:port/path?query" into (host, port, target).
// Throws PortainerError if the URL doesn't start with "https://".
static void parseHttpsUrl(const std::string& url,
                         std::string& host, std::string& port, std::string& target) {
    const std::string scheme = "https://";
    if (url.rfind(scheme, 0) != 0) {
        throw PortainerError("Portainer URL must start with https://: " + url);
    }
    auto rest = url.substr(scheme.size());
    auto slash = rest.find('/');
    auto authority = rest.substr(0, slash);
    target = (slash == std::string::npos) ? "/" : rest.substr(slash);

    auto colon = authority.find(':');
    if (colon == std::string::npos) {
        host = authority;
        port = "443";
    } else {
        host = authority.substr(0, colon);
        port = authority.substr(colon + 1);
    }
}

std::string BoostHttpClient::get(
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers) {
    std::string host, port, target;
    parseHttpsUrl(url, host, port, target);

    try {
        net::io_context ioc;
        ssl::context ctx(ssl::context::tlsv12_client);
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
            throw PortainerError("Failed to set SNI hostname");
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        for (const auto& [k, v] : headers) req.set(k, v);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);  // shutdown errors on close are common; ignore

        unsigned status = res.result_int();
        if (status < 200 || status >= 300) {
            throw PortainerError("Portainer GET " + target + " returned HTTP "
                                 + std::to_string(status));
        }
        return res.body();
    } catch (const PortainerError&) {
        throw;
    } catch (const std::exception& e) {
        throw PortainerError("HTTP GET failed for " + url + ": " + e.what());
    }
}
