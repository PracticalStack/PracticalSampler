#pragma once

#include "drs/engine/RuntimeModel.h"

#include <string>
#include <vector>

namespace drs::engine
{
struct LayerMaterializationResult
{
    bool changed = false;
    bool synthesizedDefaultGroup = false;
    bool synthesizedDefaultLayer = false;
    std::vector<std::string> notes;
};

// Repairs the hierarchy at an import/document boundary for projects using the
// layer schema. Existing authored containers and their values are preserved.
LayerMaterializationResult materializeProjectLayerHierarchy(RuntimeProjectModel& project,
                                                             bool addNotes = true);
} // namespace drs::engine
