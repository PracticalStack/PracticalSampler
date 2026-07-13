#pragma once

#include "drs/engine/EngineFacade.h"
#include "shared/PerformancePanel.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace drs::standalone
{
class MainComponent final : public juce::Component
{
public:
    MainComponent();

    void resized() override;

    std::string exportStateJson() const;
    drs::engine::EnginePresetStateRestoreResult restoreStateJson(const std::string& stateJson);
    bool setMacroValue(const std::string& macroId, double value);
    drs::engine::EngineFacade& getEngineFacade() { return engineFacade; }
    const drs::engine::EngineFacade& getEngineFacade() const { return engineFacade; }

private:
    drs::engine::EngineFacade engineFacade;
    drs::app::PerformancePanel performancePanel;
};
} // namespace drs::standalone
