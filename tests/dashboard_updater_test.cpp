#include <gtest/gtest.h>
#include "../src/dashboard/dashboard_updater.h"

class DashboardUpdaterTest : public ::testing::Test {
protected:
    // Reusable test data: a standalone stack (no containers)
    Stack netdata{"netdata", "Netdata", "mdi:chart-line", 10, {}};

    // An expander stack (with containers)
    Stack caddy{"caddy", "Caddy", "mdi:shield-check", 9, {
        {"caddy", "Caddy", "mdi:shield-check"},
        {"cloudflared", "Cloudflared", "mdi:cloud-arrow-up"}
    }};
};

TEST_F(DashboardUpdaterTest, GeneratesViewWithCorrectStructure) {
    DashboardUpdater updater("https://portainer.example.com", {netdata});

    auto view = updater.generateView();

    EXPECT_EQ(view["type"], "sections");
    EXPECT_EQ(view["path"], "test-portainer");
    EXPECT_EQ(view["title"], "Portainer");
    EXPECT_EQ(view["icon"], "mdi:docker");
    EXPECT_EQ(view["max_columns"], 4);
}

TEST_F(DashboardUpdaterTest, StandaloneStackGeneratesSection) {
    DashboardUpdater updater("https://portainer.example.com", {netdata});

    auto view = updater.generateView();
    auto& sections = view["sections"];

    // First section is always docker host, second is our stack
    ASSERT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[1]["type"], "grid");
    EXPECT_EQ(sections[1]["column_span"], 2);
}

TEST_F(DashboardUpdaterTest, ExpanderStackGeneratesSection) {
    DashboardUpdater updater("https://portainer.example.com", {caddy});

    auto view = updater.generateView();
    auto& sections = view["sections"];

    ASSERT_EQ(sections.size(), 2);

    // Expander section has an expander-card inside
    auto& cards = sections[1]["cards"];
    ASSERT_EQ(cards.size(), 1);
    EXPECT_EQ(cards[0]["type"], "custom:expander-card");
    EXPECT_EQ(cards[0]["title"], "Caddy");
}

TEST_F(DashboardUpdaterTest, BadgesIncludeHostAndStacks) {
    DashboardUpdater updater("https://portainer.example.com", {netdata, caddy});

    auto view = updater.generateView();
    auto& badges = view["badges"];

    // 3 fixed badges (host, running, stopped) + 2 stacks
    EXPECT_EQ(badges.size(), 5);
    EXPECT_EQ(badges[3]["name"], "Netdata");
    EXPECT_EQ(badges[4]["name"], "Caddy");
}

TEST_F(DashboardUpdaterTest, UpdateDashboardReplacesExistingView) {
    DashboardUpdater updater("https://portainer.example.com", {netdata});

    json existing = {
        {"views", {
            {{"path", "home"}, {"title", "Home"}},
            {{"path", "test-portainer"}, {"title", "Old Portainer"}}
        }}
    };

    auto result = updater.updateDashboard(existing);

    // Still two views — replaced, not appended
    ASSERT_EQ(result["views"].size(), 2);
    EXPECT_EQ(result["views"][0]["title"], "Home");
    EXPECT_EQ(result["views"][1]["title"], "Portainer");
    EXPECT_EQ(result["views"][1]["path"], "test-portainer");
}

TEST_F(DashboardUpdaterTest, UpdateDashboardAppendsWhenNoPortainerView) {
    DashboardUpdater updater("https://portainer.example.com", {netdata});

    json existing = {
        {"views", {
            {{"path", "home"}, {"title", "Home"}}
        }}
    };

    auto result = updater.updateDashboard(existing);

    // Now three views — original + appended
    ASSERT_EQ(result["views"].size(), 2);
    EXPECT_EQ(result["views"][0]["title"], "Home");
    EXPECT_EQ(result["views"][1]["title"], "Portainer");
}
