#include <gtest/gtest.h>
#include <sstream>
#include "../src/discovery/prompter.h"

TEST(PrompterTest, AskParsesY) {
    std::stringstream in("y\n"), out;
    EXPECT_EQ(askYesNoSkipQuit("Add?", in, out), Answer::Yes);
}

TEST(PrompterTest, AskParsesN) {
    std::stringstream in("n\n"), out;
    EXPECT_EQ(askYesNoSkipQuit("Add?", in, out), Answer::No);
}

TEST(PrompterTest, AskParsesS) {
    std::stringstream in("s\n"), out;
    EXPECT_EQ(askYesNoSkipQuit("Add?", in, out), Answer::Skip);
}

TEST(PrompterTest, AskParsesQ) {
    std::stringstream in("q\n"), out;
    EXPECT_EQ(askYesNoSkipQuit("Add?", in, out), Answer::Quit);
}

TEST(PrompterTest, AskRejectsInvalidAndRetries) {
    std::stringstream in("maybe\n\ny\n"), out;
    EXPECT_EQ(askYesNoSkipQuit("Add?", in, out), Answer::Yes);
}

TEST(PrompterTest, PromptForDiffWalksAllSectionsAndPreservesSkip) {
    DiffReport diff;
    diff.new_stacks.push_back({14, "monitoring", "monitoring"});
    diff.gone_stacks.push_back({"oldstack"});
    diff.new_containers.push_back({"immich", "immich_postgres"});
    diff.gone_containers.push_back({"immich", "old_container"});

    std::stringstream in("y\nn\ns\ny\n");
    std::stringstream out;
    auto d = promptForDiff(diff, in, out);

    ASSERT_EQ(d.new_stack.size(), 1);
    EXPECT_EQ(d.new_stack[0], Answer::Yes);
    ASSERT_EQ(d.gone_stack.size(), 1);
    EXPECT_EQ(d.gone_stack[0], Answer::No);
    ASSERT_EQ(d.new_container.size(), 1);
    EXPECT_EQ(d.new_container[0], Answer::Skip);
    ASSERT_EQ(d.gone_container.size(), 1);
    EXPECT_EQ(d.gone_container[0], Answer::Yes);
    EXPECT_FALSE(d.quit);
}

TEST(PrompterTest, AllYesProducesMatchingSizedYesVectors) {
    DiffReport diff;
    diff.new_stacks.push_back({1, "a", "a", {}});
    diff.new_stacks.push_back({2, "b", "b", {}});
    diff.gone_stacks.push_back({"old"});
    diff.new_containers.push_back({"a", "a_1"});
    diff.gone_containers.push_back({"old", "old_1"});

    auto d = allYes(diff);

    ASSERT_EQ(d.new_stack.size(), 2);
    EXPECT_EQ(d.new_stack[0], Answer::Yes);
    EXPECT_EQ(d.new_stack[1], Answer::Yes);
    ASSERT_EQ(d.gone_stack.size(), 1);
    EXPECT_EQ(d.gone_stack[0], Answer::Yes);
    ASSERT_EQ(d.new_container.size(), 1);
    EXPECT_EQ(d.new_container[0], Answer::Yes);
    ASSERT_EQ(d.gone_container.size(), 1);
    EXPECT_EQ(d.gone_container[0], Answer::Yes);
    EXPECT_FALSE(d.quit);
}

TEST(PrompterTest, QuitEndsWalkImmediately) {
    DiffReport diff;
    diff.new_stacks.push_back({1, "a", "a"});
    diff.new_stacks.push_back({2, "b", "b"});
    std::stringstream in("q\n");
    std::stringstream out;
    auto d = promptForDiff(diff, in, out);

    EXPECT_TRUE(d.quit);
}
