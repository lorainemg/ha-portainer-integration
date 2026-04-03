#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/ha/ha_client.h"

// Mock: a fake IWebSocketConnection where we control what each method does.
// MOCK_METHOD generates the fake implementation automatically.
// Format: MOCK_METHOD(ReturnType, MethodName, (params), (override))
class MockWebSocketConnection : public IWebSocketConnection {
public:
    MOCK_METHOD(void, connect, (const std::string&, const std::string&, const std::string&), (override));
    MOCK_METHOD(std::string, read, (), (override));
    MOCK_METHOD(void, write, (const std::string&), (override));
    MOCK_METHOD(void, close, (), (override));
};

// Test fixture with accessors for private members (friend class)
class HomeAssistantClientTest : public ::testing::Test {
protected:
    static const std::string& getHost(const HomeAssistantClient& c) { return c.host_; }
    static const std::string& getPort(const HomeAssistantClient& c) { return c.port_; }
    static const std::string& getPath(const HomeAssistantClient& c) { return c.path_; }
    static const std::string& getToken(const HomeAssistantClient& c) { return c.token_; }

    // Helper to create a client with a mock connection.
    // Returns both the client and a raw pointer to the mock
    // so tests can set expectations on it.
    // The unique_ptr owns the mock, but we keep a raw pointer
    // to interact with it — the raw pointer does NOT own anything.
    std::pair<HomeAssistantClient, MockWebSocketConnection*>
    makeClient(const std::string& url, const std::string& token) {
        auto mock = std::make_unique<MockWebSocketConnection>();
        auto* mock_ptr = mock.get();  // .get() returns raw pointer without transferring ownership
        return {
            HomeAssistantClient(url, token, std::move(mock)),
            mock_ptr
        };
    }
};

TEST_F(HomeAssistantClientTest, ParsesUrlWithDefaultPort) {
    auto [client, mock] = makeClient("wss://homeassistant.local/api/websocket", "test-token");

    EXPECT_EQ(getHost(client), "homeassistant.local");
    EXPECT_EQ(getPort(client), "443");
    EXPECT_EQ(getPath(client), "/api/websocket");
    EXPECT_EQ(getToken(client), "test-token");
}

TEST_F(HomeAssistantClientTest, ParsesUrlWithExplicitPort) {
    auto [client, mock] = makeClient("wss://192.168.1.100:8123/api/websocket", "my-token");

    EXPECT_EQ(getHost(client), "192.168.1.100");
    EXPECT_EQ(getPort(client), "8123");
    EXPECT_EQ(getPath(client), "/api/websocket");
}

TEST_F(HomeAssistantClientTest, ConnectAuthenticatesSuccessfully) {
    auto [client, mock] = makeClient("wss://ha.local/api/websocket", "my-token");

    // Program the mock: when connect() is called, do nothing (default).
    // When read() is called: first return auth_required, then auth_ok.
    EXPECT_CALL(*mock, connect("ha.local", "443", "/api/websocket"));
    EXPECT_CALL(*mock, read())
        .WillOnce(testing::Return(R"({"type": "auth_required"})"))
        .WillOnce(testing::Return(R"({"type": "auth_ok"})"));
    EXPECT_CALL(*mock, write(testing::_));  // _ means "any argument"

    client.connect();  // should not throw
}

TEST_F(HomeAssistantClientTest, ConnectThrowsOnAuthFailure) {
    auto [client, mock] = makeClient("wss://ha.local/api/websocket", "bad-token");

    EXPECT_CALL(*mock, connect(testing::_, testing::_, testing::_));
    EXPECT_CALL(*mock, read())
        .WillOnce(testing::Return(R"({"type": "auth_required"})"))
        .WillOnce(testing::Return(R"({"type": "auth_invalid"})"));
    EXPECT_CALL(*mock, write(testing::_));

    // EXPECT_THROW checks that the expression throws the given exception type
    EXPECT_THROW(client.connect(), std::runtime_error);
}
