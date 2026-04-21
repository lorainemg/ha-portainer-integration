#pragma once
#include <string>

// Derives an HA-safe slug from a human stack name.
// Lowercases, replaces runs of non-alphanumeric chars with a single '_',
// trims leading/trailing '_'.
//   "Home Assistant"  -> "home_assistant"
//   "Trakt TG Bot"    -> "trakt_tg_bot"
//   "my-cool-stack"   -> "my_cool_stack"
//   "  --weird!! "    -> "weird"
std::string slugify(const std::string& name);
