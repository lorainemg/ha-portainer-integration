#pragma once
#include <string>
#include <vector>

// One container that exists in Portainer but isn't in the yaml's approved list.
// Stored by stack slug + container name; the reconciler scopes the prompt accordingly.
struct NewContainer {
    std::string stack_slug;     // slug of the parent (already-approved) stack in yaml
    std::string container_name; // raw Portainer container name (becomes the slug as-is)
};

struct GoneContainer {
    std::string stack_slug;
    std::string container_slug; // slug as it currently appears in yaml
};

// One stack in Portainer that doesn't appear in either yaml.stacks or yaml.ignored.
// container_names lists the (already-slugified) container names belonging to this stack;
// when the user approves the stack, apply auto-populates these as Containers with defaults.
struct NewStack {
    int portainer_id;
    std::string portainer_name; // raw, used for default `name` and slug derivation
    std::string proposed_slug;  // slugify(portainer_name)
    std::vector<std::string> container_names;
};

struct GoneStack {
    std::string slug; // as in yaml.stacks
};

// Cases where a stack's slug already exists in yaml but the portainer_id changed.
// Applied silently — recorded for logging but never prompted.
struct IdChange {
    std::string slug;
    int old_id;
    int new_id;
};

struct DiffReport {
    std::vector<NewStack>      new_stacks;
    std::vector<GoneStack>     gone_stacks;
    std::vector<NewContainer>  new_containers;
    std::vector<GoneContainer> gone_containers;
    std::vector<IdChange>      id_changes;

    bool isEmpty() const {
        return new_stacks.empty() && gone_stacks.empty()
            && new_containers.empty() && gone_containers.empty()
            && id_changes.empty();
    }
    bool needsPrompts() const {
        return !new_stacks.empty() || !gone_stacks.empty()
            || !new_containers.empty() || !gone_containers.empty();
    }
};
