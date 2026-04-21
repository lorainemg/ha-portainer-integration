#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/portainer/portainer_client.h"
#include "../src/errors.h"

using ::testing::_;
using ::testing::Return;
using ::testing::HasSubstr;

class MockHttpClient : public IHttpClient {
public:
    MOCK_METHOD(std::string, get,
                (const std::string& url,
                 (const std::vector<std::pair<std::string, std::string>>& headers)),
                (override));
};

TEST(PortainerClientTest, ListStacksParsesIdAndName) {
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_CALL(*mock, get(HasSubstr("/api/stacks"), _))
        .WillOnce(Return(R"([
            {"Id": 1, "Name": "immich", "EndpointId": 3},
            {"Id": 9, "Name": "caddy",  "EndpointId": 3}
        ])"));

    PortainerClient client("https://portainer.local", "token", 3, std::move(mock));
    auto stacks = client.listStacks();

    ASSERT_EQ(stacks.size(), 2);
    EXPECT_EQ(stacks[0].id, 1);
    EXPECT_EQ(stacks[0].name, "immich");
    EXPECT_EQ(stacks[1].id, 9);
    EXPECT_EQ(stacks[1].name, "caddy");
}

TEST(PortainerClientTest, ListStacksSendsApiKeyHeader) {
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_CALL(*mock, get(_, ::testing::Contains(std::pair<std::string, std::string>{"X-API-Key", "secret"})))
        .WillOnce(Return("[]"));

    PortainerClient client("https://portainer.local", "secret", 3, std::move(mock));
    client.listStacks();
}

TEST(PortainerClientTest, ListAllContainersExtractsName) {
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_CALL(*mock, get(HasSubstr("/api/endpoints/3/docker/containers/json"), _))
        .WillOnce(Return(R"([
            {"Names": ["/immich_server"],   "Labels": {"com.docker.compose.project": "immich"}},
            {"Names": ["/caddy"],           "Labels": {"com.docker.compose.project": "caddy"}},
            {"Names": ["/orphan"],          "Labels": {}}
        ])"));

    PortainerClient client("https://portainer.local", "token", 3, std::move(mock));
    auto containers = client.listAllContainers();

    ASSERT_EQ(containers.size(), 3);
    EXPECT_EQ(containers[0].name, "immich_server");
    EXPECT_EQ(containers[1].name, "caddy");
    EXPECT_EQ(containers[2].name, "orphan");
}

TEST(PortainerClientTest, ListAllContainersExtractsStackNameFromLabel) {
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_CALL(*mock, get(_, _))
        .WillOnce(Return(R"([
            {"Names": ["/immich_server"], "Labels": {"com.docker.compose.project": "immich"}},
            {"Names": ["/orphan"],        "Labels": {}}
        ])"));

    PortainerClient client("https://portainer.local", "token", 3, std::move(mock));
    auto containers = client.listAllContainers();

    ASSERT_EQ(containers.size(), 2);
    EXPECT_EQ(containers[0].stack_name, "immich");
    EXPECT_EQ(containers[1].stack_name, "");
}

TEST(PortainerClientTest, ListAllContainersNormalizesHyphensInNames) {
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_CALL(*mock, get(_, _))
        .WillOnce(Return(R"([
            {"Names": ["/trakt-db"],       "Labels": {"com.docker.compose.project": "trakt-tg-bot"}},
            {"Names": ["/immich-server"],  "Labels": {"com.docker.compose.project": "immich"}}
        ])"));

    PortainerClient client("https://portainer.local", "token", 3, std::move(mock));
    auto containers = client.listAllContainers();

    ASSERT_EQ(containers.size(), 2);
    EXPECT_EQ(containers[0].name, "trakt_db");
    EXPECT_EQ(containers[1].name, "immich_server");
}

TEST(PortainerClientTest, ListStacksThrowsOnInvalidJson) {
    auto mock = std::make_unique<MockHttpClient>();
    EXPECT_CALL(*mock, get(_, _)).WillOnce(Return("not json"));
    PortainerClient client("https://portainer.local", "token", 3, std::move(mock));
    EXPECT_THROW(client.listStacks(), PortainerError);
}
