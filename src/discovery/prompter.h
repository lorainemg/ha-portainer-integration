#pragma once
#include <iostream>
#include "diff_report.h"

enum class Answer { Yes, No, Skip, Quit };

// What the user decided for each diff item.
// Indices line up with DiffReport's vectors. Each entry is one of {Yes, No, Skip}.
// Yes  -> approve change (add to stacks/containers; or remove for gone_*)
// No   -> persistently decline (slug appended to ignored / ignored_containers)
// Skip -> no yaml change this run; will be re-prompted next run
struct Decisions {
    std::vector<Answer> new_stack;
    std::vector<Answer> gone_stack;
    std::vector<Answer> new_container;
    std::vector<Answer> gone_container;
    bool quit = false;
};

// Walks the diff and prompts the user for each item via in/out streams.
Decisions promptForDiff(const DiffReport& diff,
                        std::istream& in = std::cin,
                        std::ostream& out = std::cout);

// Single-prompt helper, exposed for testing.
Answer askYesNoSkipQuit(const std::string& question,
                        std::istream& in,
                        std::ostream& out);

// Non-interactive: produce a Decisions that says Yes to every item in the diff.
// Used by --yes / CI mode to auto-accept all discovered changes.
Decisions allYes(const DiffReport& diff);
