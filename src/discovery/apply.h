#pragma once
#include "diff_report.h"
#include "prompter.h"
#include "../dashboard/stack.h"
#include "../portainer/portainer_models.h"

// Mutates `state` by applying the decisions:
//   new_stack[i] == Yes  -> append new Stack to state.stacks (default name/icon)
//   new_stack[i] == No   -> append slug to state.ignored
//   new_stack[i] == Skip -> no change this run
//   gone_stack[i] == Yes  -> remove from state.stacks
//   gone_stack[i] == No   -> leave in state.stacks
//   gone_stack[i] == Skip -> no change this run
// Containers: same shape but per-stack ignored_containers / containers.
// id_changes are always applied silently.
//
// `portainer_stacks` is unused today but kept in the signature for future extension
// (e.g. to seed initial container lists from Portainer data).
void applyDecisions(YamlState& state,
                    const DiffReport& diff,
                    const Decisions& decisions,
                    const std::vector<PortainerStack>& portainer_stacks);
