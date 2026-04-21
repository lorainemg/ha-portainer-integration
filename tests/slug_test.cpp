#include <gtest/gtest.h>
#include "../src/portainer/slug.h"

TEST(SlugTest, LowercasesAndReplacesSpaces) {
    EXPECT_EQ(slugify("Home Assistant"), "home_assistant");
}

TEST(SlugTest, ReplacesHyphensWithUnderscores) {
    EXPECT_EQ(slugify("my-cool-stack"), "my_cool_stack");
}

TEST(SlugTest, CollapsesRunsOfPunctuation) {
    EXPECT_EQ(slugify("Trakt -- TG  Bot"), "trakt_tg_bot");
}

TEST(SlugTest, TrimsLeadingAndTrailingNonAlnum) {
    EXPECT_EQ(slugify("  --weird!! "), "weird");
}

TEST(SlugTest, PreservesDigits) {
    EXPECT_EQ(slugify("immich-v2"), "immich_v2");
}

TEST(SlugTest, EmptyInputProducesEmptyOutput) {
    EXPECT_EQ(slugify(""), "");
}

TEST(SlugTest, AllPunctuationProducesEmptyOutput) {
    EXPECT_EQ(slugify("---!!!"), "");
}
