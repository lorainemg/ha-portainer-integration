#pragma once
#include "i_http_client.h"

class BoostHttpClient : public IHttpClient {
public:
    std::string get(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers) override;
};
