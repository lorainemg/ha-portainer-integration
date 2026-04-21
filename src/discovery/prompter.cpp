#include "prompter.h"
#include <string>

Answer askYesNoSkipQuit(const std::string& question,
                        std::istream& in,
                        std::ostream& out) {
    while (true) {
        out << question << " [y/n/s/q] " << std::flush;
        std::string line;
        if (!std::getline(in, line)) return Answer::Quit;
        if (line == "y" || line == "Y") return Answer::Yes;
        if (line == "n" || line == "N") return Answer::No;
        if (line == "s" || line == "S") return Answer::Skip;
        if (line == "q" || line == "Q") return Answer::Quit;
        out << "  (please answer y, n, s, or q)\n";
    }
}

// Ask one question; record into decision vector; return false if user quit.
static bool askInto(std::vector<Answer>& target,
                    const std::string& question,
                    std::istream& in,
                    std::ostream& out,
                    Decisions& d) {
    Answer a = askYesNoSkipQuit(question, in, out);
    if (a == Answer::Quit) { d.quit = true; return false; }
    target.push_back(a);
    return true;
}

Decisions promptForDiff(const DiffReport& diff,
                        std::istream& in,
                        std::ostream& out) {
    Decisions d;
    out << "\nDetected changes vs. Portainer:\n";

    for (const auto& s : diff.new_stacks) {
        std::string q = "[NEW STACK] \"" + s.portainer_name
                      + "\" (id=" + std::to_string(s.portainer_id)
                      + ", slug=" + s.proposed_slug;
        if (!s.container_names.empty()) {
            q += ", containers=";
            for (size_t i = 0; i < s.container_names.size(); ++i) {
                if (i > 0) q += ",";
                q += s.container_names[i];
            }
        }
        q += ") — add to dashboard?";
        if (!askInto(d.new_stack, q, in, out, d)) return d;
    }
    for (const auto& s : diff.gone_stacks) {
        std::string q = "[GONE STACK] \"" + s.slug + "\" no longer in Portainer — remove from yaml?";
        if (!askInto(d.gone_stack, q, in, out, d)) return d;
    }
    for (const auto& c : diff.new_containers) {
        std::string q = "[NEW CONTAINER] \"" + c.container_name
                      + "\" in stack \"" + c.stack_slug + "\" — add to dashboard?";
        if (!askInto(d.new_container, q, in, out, d)) return d;
    }
    for (const auto& c : diff.gone_containers) {
        std::string q = "[GONE CONTAINER] \"" + c.container_slug
                      + "\" gone from stack \"" + c.stack_slug + "\" — remove from yaml?";
        if (!askInto(d.gone_container, q, in, out, d)) return d;
    }
    return d;
}
