#include <gtest/gtest.h>
#include <fstream>
#include "../src/config/config_parser.h"
#include "../src/errors.h"

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

    auto stacks = loadYamlState(path).stacks;

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

    auto stacks = loadYamlState(path).stacks;

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

    auto stacks = loadYamlState(path).stacks;

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

    auto stacks = loadYamlState(path).stacks;

    ASSERT_EQ(stacks.size(), 3);
    EXPECT_EQ(stacks[0].slug, "netdata");
    EXPECT_EQ(stacks[1].slug, "portainer");
    EXPECT_EQ(stacks[2].slug, "caddy");
    EXPECT_FALSE(stacks[0].isExpander());
    EXPECT_TRUE(stacks[2].isExpander());
}

TEST(ConfigParserTest, ThrowsConfigErrorOnMissingFile) {
    EXPECT_THROW(loadYamlState("/tmp/nonexistent_stacks.yaml"), ConfigError);
}

TEST(ConfigParserTest, ThrowsConfigErrorOnInvalidYaml) {
    auto path = writeTempYaml("not: [valid: yaml: content");
    EXPECT_THROW(loadYamlState(path), ConfigError);
}

TEST(ConfigParserTest, ThrowsConfigErrorOnMissingSlug) {
    auto path = writeTempYaml(R"(
stacks:
  - name: Netdata
    icon: mdi:chart-line
)");
    EXPECT_THROW(loadYamlState(path), ConfigError);
}

TEST(ConfigParserTest, ThrowsConfigErrorOnMissingName) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: netdata
    icon: mdi:chart-line
)");
    EXPECT_THROW(loadYamlState(path), ConfigError);
}

TEST(ConfigParserTest, ThrowsConfigErrorOnMissingIcon) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: netdata
    name: Netdata
)");
    EXPECT_THROW(loadYamlState(path), ConfigError);
}

TEST(ConfigParserTest, ThrowsConfigErrorOnMissingContainerSlug) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: caddy
    name: Caddy
    icon: mdi:shield-check
    containers:
      - name: Caddy
        icon: mdi:shield-check
)");
    EXPECT_THROW(loadYamlState(path), ConfigError);
}

TEST(ConfigParserTest, LoadsTopLevelIgnoredList) {
    auto path = writeTempYaml(R"(
stacks: []
ignored:
  - watchtower
  - dev_only_stack
)");
    auto state = loadYamlState(path);
    ASSERT_EQ(state.ignored.size(), 2);
    EXPECT_EQ(state.ignored[0], "watchtower");
    EXPECT_EQ(state.ignored[1], "dev_only_stack");
}

TEST(ConfigParserTest, LoadsPerStackIgnoredContainers) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: immich
    name: Immich
    icon: mdi:image-multiple
    portainer_id: 1
    containers:
      - slug: immich_server
        name: Server
        icon: mdi:server
    ignored_containers:
      - immich_redis
      - immich_typesense
)");
    auto state = loadYamlState(path);
    ASSERT_EQ(state.stacks.size(), 1);
    ASSERT_EQ(state.stacks[0].ignored_containers.size(), 2);
    EXPECT_EQ(state.stacks[0].ignored_containers[0], "immich_redis");
}

TEST(ConfigParserTest, IgnoredFieldsDefaultToEmpty) {
    auto path = writeTempYaml(R"(
stacks:
  - slug: netdata
    name: Netdata
    icon: mdi:chart-line
    portainer_id: 10
)");
    auto state = loadYamlState(path);
    EXPECT_TRUE(state.ignored.empty());
    EXPECT_TRUE(state.stacks[0].ignored_containers.empty());
}

TEST(ConfigParserTest, RoundTripPreservesAllFields) {
    YamlState original;
    Stack s1{.slug = "immich",
             .name = "Immich",
             .icon = "mdi:image-multiple",
             .portainer_id = 1};
    s1.containers.push_back({"immich_server", "Server", "mdi:server"});
    s1.containers.push_back({"immich_postgres", "Postgres", "mdi:database"});
    s1.ignored_containers = {"immich_redis"};
    original.stacks.push_back(s1);
    original.ignored = {"watchtower"};

    std::string path = "/tmp/test_roundtrip.yaml";
    saveYamlState(path, original);

    auto reloaded = loadYamlState(path);

    ASSERT_EQ(reloaded.stacks.size(), 1);
    EXPECT_EQ(reloaded.stacks[0].slug, "immich");
    EXPECT_EQ(reloaded.stacks[0].name, "Immich");
    EXPECT_EQ(reloaded.stacks[0].icon, "mdi:image-multiple");
    EXPECT_EQ(reloaded.stacks[0].portainer_id, 1);
    ASSERT_EQ(reloaded.stacks[0].containers.size(), 2);
    EXPECT_EQ(reloaded.stacks[0].containers[1].name, "Postgres");
    ASSERT_EQ(reloaded.stacks[0].ignored_containers.size(), 1);
    EXPECT_EQ(reloaded.stacks[0].ignored_containers[0], "immich_redis");
    ASSERT_EQ(reloaded.ignored.size(), 1);
    EXPECT_EQ(reloaded.ignored[0], "watchtower");
}

TEST(ConfigParserTest, SaveOmitsEmptyOptionalFields) {
    YamlState original;
    original.stacks.push_back({.slug = "netdata",
                               .name = "Netdata",
                               .icon = "mdi:chart-line",
                               .portainer_id = 10});
    saveYamlState("/tmp/test_minimal.yaml", original);
    auto reloaded = loadYamlState("/tmp/test_minimal.yaml");

    EXPECT_TRUE(reloaded.stacks[0].containers.empty());
    EXPECT_TRUE(reloaded.stacks[0].ignored_containers.empty());
    EXPECT_TRUE(reloaded.ignored.empty());
}
