#pragma once

#include "shared/authoring/StructureInspector.h"

namespace drs::app::authoring
{
// Stable seam name for future inspector dispatch expansion. The first
// implementation is intentionally a single context-sensitive inspector so
// Map and Structure share one right-hand host slot.
using StructureInspectorHost = StructureInspector;
} // namespace drs::app::authoring
