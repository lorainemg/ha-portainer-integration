#include <gtest/gtest.h>
#include "../src/discovery/apply.h"

TEST(ApplyTest, ApprovedNewStackIsAppendedWithDefaults) {
    YamlState state;
    DiffReport diff;
    diff.new_stacks.push_back({14, "monitoring", "monitoring"});

    Decisions d;
    d.new_stack = {Answer::Yes};

    std::vector<PortainerStack> p = {{14, "monitoring"}};
    applyDecisions(state, diff, d, p);

    ASSERT_EQ(state.stacks.size(), 1);
    EXPECT_EQ(state.stacks[0].slug, "monitoring");
    EXPECT_EQ(state.stacks[0].name, "monitoring");
    EXPECT_EQ(state.stacks[0].icon, "mdi:cube-outline");
    EXPECT_EQ(state.stacks[0].portainer_id, 14);
    EXPECT_TRUE(state.ignored.empty());
}

TEST(ApplyTest, DeclinedNewStackGoesToIgnored) {
    YamlState state;
    DiffReport diff;
    diff.new_stacks.push_back({15, "watchtower", "watchtower"});

    Decisions d;
    d.new_stack = {Answer::No};
    applyDecisions(state, diff, d, {{15, "watchtower"}});

    EXPECT_TRUE(state.stacks.empty());
    ASSERT_EQ(state.ignored.size(), 1);
    EXPECT_EQ(state.ignored[0], "watchtower");
}

TEST(ApplyTest, SkippedNewStackProducesNoChange) {
    YamlState state;
    DiffReport diff;
    diff.new_stacks.push_back({15, "watchtower", "watchtower"});

    Decisions d;
    d.new_stack = {Answer::Skip};
    applyDecisions(state, diff, d, {{15, "watchtower"}});

    EXPECT_TRUE(state.stacks.empty());
    EXPECT_TRUE(state.ignored.empty());  // crucially: NOT added to ignored
}

TEST(ApplyTest, ApprovedGoneStackIsRemoved) {
    YamlState state;
    state.stacks.push_back({.slug = "old", .name = "Old", .icon = "x", .portainer_id = 99});

    DiffReport diff;
    diff.gone_stacks.push_back({"old"});

    Decisions d;
    d.gone_stack = {Answer::Yes};
    applyDecisions(state, diff, d, {});

    EXPECT_TRUE(state.stacks.empty());
}

TEST(ApplyTest, ApprovedNewContainerIsAppended) {
    YamlState state;
    state.stacks.push_back({.slug = "immich", .name = "Immich", .icon = "x", .portainer_id = 1});

    DiffReport diff;
    diff.new_containers.push_back({"immich", "immich_postgres"});

    Decisions d;
    d.new_container = {Answer::Yes};
    applyDecisions(state, diff, d, {{1, "immich"}});

    ASSERT_EQ(state.stacks[0].containers.size(), 1);
    EXPECT_EQ(state.stacks[0].containers[0].slug, "immich_postgres");
    EXPECT_EQ(state.stacks[0].containers[0].name, "immich_postgres");
    EXPECT_EQ(state.stacks[0].containers[0].icon, "mdi:application-outline");
}

TEST(ApplyTest, DeclinedNewContainerGoesToStackIgnoredList) {
    YamlState state;
    state.stacks.push_back({.slug = "immich", .name = "Immich", .icon = "x", .portainer_id = 1});

    DiffReport diff;
    diff.new_containers.push_back({"immich", "immich_redis"});

    Decisions d;
    d.new_container = {Answer::No};
    applyDecisions(state, diff, d, {{1, "immich"}});

    EXPECT_TRUE(state.stacks[0].containers.empty());
    ASSERT_EQ(state.stacks[0].ignored_containers.size(), 1);
    EXPECT_EQ(state.stacks[0].ignored_containers[0], "immich_redis");
}

TEST(ApplyTest, SkippedNewContainerProducesNoChange) {
    YamlState state;
    state.stacks.push_back({.slug = "immich", .name = "Immich", .icon = "x", .portainer_id = 1});

    DiffReport diff;
    diff.new_containers.push_back({"immich", "immich_redis"});

    Decisions d;
    d.new_container = {Answer::Skip};
    applyDecisions(state, diff, d, {{1, "immich"}});

    EXPECT_TRUE(state.stacks[0].containers.empty());
    EXPECT_TRUE(state.stacks[0].ignored_containers.empty());
}

TEST(ApplyTest, IdChangesAreAppliedSilently) {
    YamlState state;
    state.stacks.push_back({.slug = "immich", .name = "Immich", .icon = "x", .portainer_id = 1});

    DiffReport diff;
    diff.id_changes.push_back({"immich", 1, 42});

    Decisions d;
    applyDecisions(state, diff, d, {});

    EXPECT_EQ(state.stacks[0].portainer_id, 42);
}
