#include "reconciler.h"
#include "../portainer/slug.h"
#include <unordered_map>
#include <unordered_set>

// Finds an approved stack in the yaml by slug; returns nullptr if not present.
static const Stack* findStack(const YamlState& yaml, const std::string& slug) {
    for (const auto& s : yaml.stacks) {
        if (s.slug == slug) return &s;
    }
    return nullptr;
}

DiffReport reconcile(const YamlState& yaml,
                     const std::vector<PortainerStack>& portainer_stacks,
                     const std::vector<PortainerContainer>& portainer_containers) {
    DiffReport diff;

    std::unordered_set<std::string> portainer_slugs;
    for (const auto& p : portainer_stacks) portainer_slugs.insert(slugify(p.name));

    std::unordered_set<std::string> yaml_slugs;
    for (const auto& s : yaml.stacks) yaml_slugs.insert(s.slug);

    std::unordered_set<std::string> ignored_set(yaml.ignored.begin(), yaml.ignored.end());

    // New stacks + id changes — walk Portainer's list
    for (const auto& p : portainer_stacks) {
        std::string slug = slugify(p.name);
        if (yaml_slugs.count(slug)) {
            const Stack* existing = findStack(yaml, slug);
            if (existing && existing->portainer_id != p.id) {
                diff.id_changes.push_back({slug, existing->portainer_id, p.id});
            }
        } else if (!ignored_set.count(slug)) {
            NewStack ns{p.id, p.name, slug, {}};
            for (const auto& c : portainer_containers) {
                if (slugify(c.stack_name) == slug) {
                    ns.container_names.push_back(c.name);
                }
            }
            diff.new_stacks.push_back(std::move(ns));
        }
    }

    // Gone stacks — yaml entries not in Portainer
    for (const auto& s : yaml.stacks) {
        if (!portainer_slugs.count(s.slug)) {
            diff.gone_stacks.push_back({s.slug});
        }
    }

    // New containers — Portainer containers whose parent stack is approved and not listed
    for (const auto& c : portainer_containers) {
        const Stack* parent = findStack(yaml, slugify(c.stack_name));
        if (!parent) continue;

        bool in_containers = false;
        for (const auto& yc : parent->containers) {
            if (yc.slug == c.name) { in_containers = true; break; }
        }
        if (in_containers) continue;

        bool is_ignored = false;
        for (const auto& ig : parent->ignored_containers) {
            if (ig == c.name) { is_ignored = true; break; }
        }
        if (is_ignored) continue;

        diff.new_containers.push_back({parent->slug, c.name});
    }

    // Gone containers — yaml entries no longer in Portainer under their parent stack.
    // Composite key uses \x1f (unit separator) to avoid collisions with any real slug content.
    std::unordered_set<std::string> portainer_container_keys;
    for (const auto& c : portainer_containers) {
        portainer_container_keys.insert(slugify(c.stack_name) + "\x1f" + c.name);
    }
    for (const auto& s : yaml.stacks) {
        for (const auto& c : s.containers) {
            if (!portainer_container_keys.count(s.slug + "\x1f" + c.slug)) {
                diff.gone_containers.push_back({s.slug, c.slug});
            }
        }
    }

    return diff;
}
