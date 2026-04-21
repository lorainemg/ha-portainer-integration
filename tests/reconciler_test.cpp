#include <gtest/gtest.h>
#include "../src/discovery/reconciler.h"

// Helper: build an approved stack with optional containers.
static Stack approvedStack(const std::string& slug, int id,
                           std::vector<Container> containers = {}) {
    Stack s{.slug = slug, .name = slug, .icon = "x", .portainer_id = id};
    s.containers = std::move(containers);
    return s;
}

TEST(ReconcilerTest, EmptyDiffWhenStateMatchesPortainer) {
    YamlState yaml;
    yaml.stacks.push_back(approvedStack("immich", 1, {{"immich_server", "Server", "x"}}));

    std::vector<PortainerStack>     pStacks     = {{1, "immich"}};
    std::vector<PortainerContainer> pContainers = {{"immich_server", "immich", 0}};

    auto diff = reconcile(yaml, pStacks, pContainers);

    EXPECT_TRUE(diff.isEmpty());
}

TEST(ReconcilerTest, NewStackAppearsInDiff) {
    YamlState yaml;
    std::vector<PortainerStack>     pStacks     = {{14, "monitoring"}};
    std::vector<PortainerContainer> pContainers = {};

    auto diff = reconcile(yaml, pStacks, pContainers);

    ASSERT_EQ(diff.new_stacks.size(), 1);
    EXPECT_EQ(diff.new_stacks[0].portainer_id, 14);
    EXPECT_EQ(diff.new_stacks[0].portainer_name, "monitoring");
    EXPECT_EQ(diff.new_stacks[0].proposed_slug, "monitoring");
    EXPECT_TRUE(diff.new_stacks[0].container_names.empty());
    EXPECT_TRUE(diff.gone_stacks.empty());
}

TEST(ReconcilerTest, NewStackIncludesItsContainers) {
    YamlState yaml;
    std::vector<PortainerStack>     pStacks     = {{14, "monitoring"}};
    std::vector<PortainerContainer> pContainers = {
        {"grafana",    "monitoring", 0},
        {"prometheus", "monitoring", 0},
        {"caddy",      "caddy",      0},  // belongs to a different stack — should not attach
    };

    auto diff = reconcile(yaml, pStacks, pContainers);

    ASSERT_EQ(diff.new_stacks.size(), 1);
    ASSERT_EQ(diff.new_stacks[0].container_names.size(), 2);
    EXPECT_EQ(diff.new_stacks[0].container_names[0], "grafana");
    EXPECT_EQ(diff.new_stacks[0].container_names[1], "prometheus");
    // new_containers is strictly for additions to already-approved stacks;
    // a brand-new stack's containers ride along inside NewStack.container_names.
    EXPECT_TRUE(diff.new_containers.empty());
}

TEST(ReconcilerTest, IgnoredStackDoesNotAppearAsNew) {
    YamlState yaml;
    yaml.ignored = {"monitoring"};
    std::vector<PortainerStack>     pStacks     = {{14, "monitoring"}};
    std::vector<PortainerContainer> pContainers = {};

    auto diff = reconcile(yaml, pStacks, pContainers);

    EXPECT_TRUE(diff.new_stacks.empty());
}

TEST(ReconcilerTest, GoneStackAppearsInDiff) {
    YamlState yaml;
    yaml.stacks.push_back(approvedStack("netdata", 10));

    std::vector<PortainerStack>     pStacks     = {};
    std::vector<PortainerContainer> pContainers = {};

    auto diff = reconcile(yaml, pStacks, pContainers);

    ASSERT_EQ(diff.gone_stacks.size(), 1);
    EXPECT_EQ(diff.gone_stacks[0].slug, "netdata");
}

TEST(ReconcilerTest, NewContainerWithinApprovedStack) {
    YamlState yaml;
    yaml.stacks.push_back(approvedStack("immich", 1, {{"immich_server", "Server", "x"}}));

    std::vector<PortainerStack>     pStacks     = {{1, "immich"}};
    std::vector<PortainerContainer> pContainers = {
        {"immich_server",  "immich", 0},
        {"immich_postgres","immich", 0},
    };

    auto diff = reconcile(yaml, pStacks, pContainers);

    ASSERT_EQ(diff.new_containers.size(), 1);
    EXPECT_EQ(diff.new_containers[0].stack_slug, "immich");
    EXPECT_EQ(diff.new_containers[0].container_name, "immich_postgres");
}

TEST(ReconcilerTest, IgnoredContainerDoesNotAppearAsNew) {
    YamlState yaml;
    Stack s = approvedStack("immich", 1, {{"immich_server", "Server", "x"}});
    s.ignored_containers = {"immich_redis"};
    yaml.stacks.push_back(s);

    std::vector<PortainerStack>     pStacks     = {{1, "immich"}};
    std::vector<PortainerContainer> pContainers = {
        {"immich_server", "immich", 0},
        {"immich_redis",  "immich", 0},
    };

    auto diff = reconcile(yaml, pStacks, pContainers);

    EXPECT_TRUE(diff.new_containers.empty());
}

TEST(ReconcilerTest, GoneContainerAppearsInDiff) {
    YamlState yaml;
    yaml.stacks.push_back(approvedStack("immich", 1,
        {{"immich_server", "Server", "x"}, {"immich_postgres", "Postgres", "x"}}));

    std::vector<PortainerStack>     pStacks     = {{1, "immich"}};
    std::vector<PortainerContainer> pContainers = {{"immich_server", "immich", 0}};

    auto diff = reconcile(yaml, pStacks, pContainers);

    ASSERT_EQ(diff.gone_containers.size(), 1);
    EXPECT_EQ(diff.gone_containers[0].stack_slug, "immich");
    EXPECT_EQ(diff.gone_containers[0].container_slug, "immich_postgres");
}

TEST(ReconcilerTest, IdChangeIsRecordedSilently) {
    YamlState yaml;
    yaml.stacks.push_back(approvedStack("immich", 1));

    std::vector<PortainerStack>     pStacks     = {{42, "immich"}};
    std::vector<PortainerContainer> pContainers = {};

    auto diff = reconcile(yaml, pStacks, pContainers);

    ASSERT_EQ(diff.id_changes.size(), 1);
    EXPECT_EQ(diff.id_changes[0].slug, "immich");
    EXPECT_EQ(diff.id_changes[0].old_id, 1);
    EXPECT_EQ(diff.id_changes[0].new_id, 42);
    EXPECT_TRUE(diff.new_stacks.empty());
    EXPECT_TRUE(diff.gone_stacks.empty());
    EXPECT_FALSE(diff.isEmpty());
    EXPECT_FALSE(diff.needsPrompts());
}
