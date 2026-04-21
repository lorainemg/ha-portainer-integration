#pragma once
#include <string>
#include <vector>
#include <utility>

// Pure-virtual HTTPS GET interface — the abstraction we mock in tests.
class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    // Performs HTTPS GET. Throws PortainerError on connection failure or non-2xx response.
    // headers: list of {name, value} pairs added to the request.
    // Returns the response body as a string.
    virtual std::string get(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers) = 0;
};
