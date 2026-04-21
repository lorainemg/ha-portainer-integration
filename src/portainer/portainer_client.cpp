#include "portainer_client.h"
#include "../errors.h"
#include "slug.h"
#include <nlohmann/json.hpp>
#include <utility>

using json = nlohmann::json;

PortainerClient::PortainerClient(std::string base_url,
                                 std::string api_token,
                                 int endpoint_id,
                                 std::unique_ptr<IHttpClient> http)
    : base_url_(std::move(base_url)),
      api_token_(std::move(api_token)),
      endpoint_id_(endpoint_id),
      http_(std::move(http)) {}

static std::vector<std::pair<std::string, std::string>> authHeaders(const std::string& token) {
    return {{"X-API-Key", token},
            {"Accept", "application/json"}};
}

std::vector<PortainerStack> PortainerClient::listStacks() {
    std::string url = base_url_ + "/api/stacks?filters=%7B%22EndpointId%22%3A"
                    + std::to_string(endpoint_id_) + "%7D";
    std::string body = http_->get(url, authHeaders(api_token_));

    json parsed;
    try {
        parsed = json::parse(body);
    } catch (const json::parse_error& e) {
        throw PortainerError(std::string("Failed to parse stacks response: ") + e.what());
    }
    if (!parsed.is_array()) {
        throw PortainerError("Expected JSON array from /api/stacks");
    }

    std::vector<PortainerStack> out;
    for (const auto& item : parsed) {
        PortainerStack s;
        s.id   = item.value("Id", 0);
        s.name = item.value("Name", "");
        if (s.id == 0 || s.name.empty()) continue;
        out.push_back(s);
    }
    return out;
}

std::vector<PortainerContainer> PortainerClient::listAllContainers() {
    std::string url = base_url_ + "/api/endpoints/" + std::to_string(endpoint_id_)
                    + "/docker/containers/json?all=true";
    std::string body = http_->get(url, authHeaders(api_token_));

    json parsed;
    try {
        parsed = json::parse(body);
    } catch (const json::parse_error& e) {
        throw PortainerError(std::string("Failed to parse containers response: ") + e.what());
    }
    if (!parsed.is_array()) {
        throw PortainerError("Expected JSON array from /docker/containers/json");
    }

    std::vector<PortainerContainer> out;
    for (const auto& item : parsed) {
        PortainerContainer c;
        if (item.contains("Names") && item["Names"].is_array() && !item["Names"].empty()) {
            std::string raw = item["Names"][0].get<std::string>();
            std::string stripped = (!raw.empty() && raw[0] == '/') ? raw.substr(1) : raw;
            // Normalize hyphens to underscores so the slug matches HA entity conventions
            // and stays consistent with yaml's stored container slugs.
            c.name = slugify(stripped);
        }
        if (item.contains("Labels") && item["Labels"].is_object()) {
            const auto& labels = item["Labels"];
            if (labels.contains("com.docker.compose.project")) {
                c.stack_name = labels["com.docker.compose.project"].get<std::string>();
            }
        }
        if (!c.name.empty()) out.push_back(c);
    }
    return out;
}
