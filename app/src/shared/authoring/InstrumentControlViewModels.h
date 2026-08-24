#pragma once

#include "drs/engine/InstrumentControlContract.h"

#include <string>
#include <utility>
#include <vector>

namespace drs::app::authoring
{
struct InstrumentControlViewRow
{
    std::string id;
    std::string title;
    std::string category;
    std::string valueText;
    std::string defaultText;
    std::string sourceText;
    std::string provenanceText;
    std::string accessibleText;
    bool mixerSurface = false;
    bool conflict = false;
};

struct InstrumentControlAssignmentViewRow
{
    std::string id;
    std::string destinationText;
    std::string sourceText;
    std::string statusText;
    std::string accessibleText;
    bool imported = false;
    bool conflict = false;
};

std::vector<InstrumentControlViewRow> buildInstrumentControlViewRows(
    const std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition>& bindings,
    const std::vector<std::pair<std::string, double>>& currentValues = {});

std::vector<InstrumentControlAssignmentViewRow> buildInstrumentControlAssignmentViewRows(
    const std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition>& bindings);
} // namespace drs::app::authoring
