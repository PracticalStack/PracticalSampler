#pragma once

#include "shared/authoring/StructureInspector.h"

namespace drs::app::authoring
{
// Explicit host type keeps dispatch ownership separate from the editable
// inspector implementation. It currently forwards to the single context
// inspector while preserving one authoritative right-hand slot.
class StructureInspectorHost final : public StructureInspector
{
public:
    using StructureInspector::StructureInspector;
};
} // namespace drs::app::authoring
