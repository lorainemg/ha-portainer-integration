#include <gtest/gtest.h>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include "../src/config/config_parser.h"

// Helper: writes YAML content to a temp file and returns the path
static std::string writeTempYaml(const std::string& content) {
    std::string path = "/tmp/test_stacks.yaml";
    std::ofstream out(path);
    out << content;
    return path;
}

TEST(ConfigParserTest, LoadsStandaloneStack) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: netdata
    name: Netdata
    icon: mdi:chart-line
    portainer_id: 10
)");

    auto stacks = loadStacks(path);

    ASSERT_EQ(stacks.size(), 1);
    EXPECT_EQ(stacks[0].slug, "netdata");
    EXPECT_EQ(stacks[0].name, "Netdata");
    EXPECT_EQ(stacks[0].icon, "mdi:chart-line");
    EXPECT_EQ(stacks[0].portainer_id, 10);
    EXPECT_TRUE(stacks[0].containers.empty());
}

TEST(ConfigParserTest, LoadsStackWithContainers) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: caddy
    name: Caddy
    icon: mdi:image-multiple
    portainer_id: 9
    containers:
      - slug: caddy
        name: Caddy
        icon: mdi:shield-check
      - slug: cloudflared
        name: Cloudflared
        icon: mdi:cloud-arrow-up
)");

    auto stacks = loadStacks(path);

    ASSERT_EQ(stacks.size(), 1);
    ASSERT_EQ(stacks[0].containers.size(), 2);
    EXPECT_EQ(stacks[0].containers[0].slug, "caddy");
    EXPECT_EQ(stacks[0].containers[1].slug, "cloudflared");
    EXPECT_EQ(stacks[0].containers[1].name, "Cloudflared");
}

TEST(ConfigParserTest, DefaultsPortainerIdToZero) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: portainer
    name: Portainer
    icon: mdi:anchor
)");

    auto stacks = loadStacks(path);

    ASSERT_EQ(stacks.size(), 1);
    EXPECT_EQ(stacks[0].portainer_id, 0);
}

TEST(ConfigParserTest, LoadsMultipleStacks) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: netdata
    name: Netdata
    icon: mdi:chart-line
    portainer_id: 10
  - slug: portainer
    name: Portainer
    icon: mdi:anchor
  - slug: caddy
    name: Caddy
    icon: mdi:image-multiple
    portainer_id: 9
    containers:
      - slug: caddy
        name: Caddy
        icon: mdi:shield-check
)");

    auto stacks = loadStacks(path);

    ASSERT_EQ(stacks.size(), 3);
    EXPECT_EQ(stacks[0].slug, "netdata");
    EXPECT_EQ(stacks[1].slug, "portainer");
    EXPECT_EQ(stacks[2].slug, "caddy");
    EXPECT_FALSE(stacks[0].isExpander());
    EXPECT_TRUE(stacks[2].isExpander());
}

TEST(ConfigParserTest, ThrowsOnMissingFile) {
    EXPECT_THROW(loadStacks("/tmp/nonexistent.yaml"), YAML::BadFile);
}
