#pragma once
#include <vector>
#include "diff_report.h"
#include "../dashboard/stack.h"
#include "../portainer/portainer_models.h"

// Pure function: compares the persisted YAML state against Portainer reality
// and returns a DiffReport describing all the differences.
DiffReport reconcile(const YamlState& yaml,
                     const std::vector<PortainerStack>& portainer_stacks,
                     const std::vector<PortainerContainer>& portainer_containers);
