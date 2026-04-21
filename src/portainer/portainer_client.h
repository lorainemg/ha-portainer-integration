#pragma once
#include <memory>
#include <string>
#include <vector>
#include "i_http_client.h"
#include "portainer_models.h"

class PortainerClient {
public:
    PortainerClient(std::string base_url,
                    std::string api_token,
                    int endpoint_id,
                    std::unique_ptr<IHttpClient> http);

    // GET /api/stacks?filters={"EndpointId":N}
    std::vector<PortainerStack> listStacks();

    // GET /api/endpoints/N/docker/containers/json?all=true
    // Returns ALL containers on the endpoint; reconciler filters by stack_name later.
    std::vector<PortainerContainer> listAllContainers();

private:
    std::string base_url_;
    std::string api_token_;
    int endpoint_id_;
    std::unique_ptr<IHttpClient> http_;
};
