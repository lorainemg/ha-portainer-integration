#include "apply.h"
#include <algorithm>

static const char* kDefaultStackIcon     = "mdi:cube-outline";
static const char* kDefaultContainerIcon = "mdi:application-outline";

static Stack* findStackBySlug(YamlState& state, const std::string& slug) {
    for (auto& s : state.stacks) if (s.slug == slug) return &s;
    return nullptr;
}

// Returns the user's answer for index i, or Skip if the vector is shorter than expected (defensive).
static Answer answerAt(const std::vector<Answer>& v, size_t i) {
    return i < v.size() ? v[i] : Answer::Skip;
}

void applyDecisions(YamlState& state,
                    const DiffReport& diff,
                    const Decisions& decisions,
                    const std::vector<PortainerStack>& /*portainer_stacks*/) {
    for (size_t i = 0; i < diff.new_stacks.size(); ++i) {
        const auto& ns = diff.new_stacks[i];
        switch (answerAt(decisions.new_stack, i)) {
            case Answer::Yes: {
                Stack new_stack{.slug = ns.proposed_slug,
                                .name = ns.portainer_name,
                                .icon = kDefaultStackIcon,
                                .portainer_id = ns.portainer_id};
                for (const auto& cname : ns.container_names) {
                    new_stack.containers.push_back({cname, cname, kDefaultContainerIcon});
                }
                state.stacks.push_back(std::move(new_stack));
                break;
            }
            case Answer::No:
                state.ignored.push_back(ns.proposed_slug);
                break;
            case Answer::Skip:
            case Answer::Quit:  // unreachable here — main aborts before calling apply on quit
                break;
        }
    }

    for (size_t i = 0; i < diff.gone_stacks.size(); ++i) {
        if (answerAt(decisions.gone_stack, i) != Answer::Yes) continue;
        const auto& slug = diff.gone_stacks[i].slug;
        state.stacks.erase(
            std::remove_if(state.stacks.begin(), state.stacks.end(),
                           [&](const Stack& s) { return s.slug == slug; }),
            state.stacks.end());
    }

    for (size_t i = 0; i < diff.new_containers.size(); ++i) {
        const auto& nc = diff.new_containers[i];
        Stack* parent = findStackBySlug(state, nc.stack_slug);
        if (!parent) continue;  // parent stack was removed earlier in this pass
        switch (answerAt(decisions.new_container, i)) {
            case Answer::Yes:
                parent->containers.push_back({nc.container_name, nc.container_name, kDefaultContainerIcon});
                break;
            case Answer::No:
                parent->ignored_containers.push_back(nc.container_name);
                break;
            case Answer::Skip:
            case Answer::Quit:
                break;
        }
    }

    for (size_t i = 0; i < diff.gone_containers.size(); ++i) {
        if (answerAt(decisions.gone_container, i) != Answer::Yes) continue;
        const auto& gc = diff.gone_containers[i];
        Stack* parent = findStackBySlug(state, gc.stack_slug);
        if (!parent) continue;
        parent->containers.erase(
            std::remove_if(parent->containers.begin(), parent->containers.end(),
                           [&](const Container& c) { return c.slug == gc.container_slug; }),
            parent->containers.end());
    }

    for (const auto& change : diff.id_changes) {
        if (auto* s = findStackBySlug(state, change.slug)) {
            s->portainer_id = change.new_id;
        }
    }
}
