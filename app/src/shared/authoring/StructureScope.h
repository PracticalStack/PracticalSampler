#pragma once

#include "shared/authoring/AuthoringStructureSelection.h"
#include "drs/engine/RuntimeModel.h"

#include <string>
#include <vector>

namespace drs::app::authoring
{
// The instrument root is a workspace identity, not an authored project field.
inline constexpr const char* kInstrumentStructureId = "instrument";

enum class StructureScopeKind
{
    instrument,
    layer,
    group
};

struct StructureScope
{
    StructureScopeKind kind = StructureScopeKind::instrument;
    std::string id = kInstrumentStructureId;

    bool operator==(const StructureScope& other) const noexcept
    {
        return kind == other.kind && id == other.id;
    }
    bool operator!=(const StructureScope& other) const noexcept { return !(*this == other); }
};

bool isValidStructureScope(const drs::engine::RuntimeProjectModel& project,
                           const StructureScope& scope) noexcept;

StructureScope makeInstrumentStructureScope();

// Selection-to-scope policy deliberately differs from selection itself:
// selecting a zone does not change the Map input until the user invokes Show
// Zones. A zone selection only supplies the nearest useful parent scope.
StructureScope deriveStructureScope(const drs::engine::RuntimeProjectModel& project,
                                    const AuthoringStructureSelection& selection);

StructureScope reconcileStructureScope(const drs::engine::RuntimeProjectModel& project,
                                       const StructureScope& previous);

std::string structureScopeName(const drs::engine::RuntimeProjectModel& project,
                               const StructureScope& scope);

std::vector<std::string> zoneIdsInStructureScope(const drs::engine::RuntimeProjectModel& project,
                                                  const StructureScope& scope);
} // namespace drs::app::authoring
