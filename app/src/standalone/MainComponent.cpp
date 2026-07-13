#include "standalone/MainComponent.h"

namespace drs::standalone
{
MainComponent::MainComponent()
    : performancePanel(engineFacade)
{
    addAndMakeVisible(performancePanel);
    setSize(860, 760);
}

void MainComponent::resized()
{
    performancePanel.setBounds(getLocalBounds());
}

std::string MainComponent::exportStateJson() const
{
    return engineFacade.exportPresetStateJson();
}

drs::engine::EnginePresetStateRestoreResult MainComponent::restoreStateJson(const std::string& stateJson)
{
    return engineFacade.restorePresetStateJson(stateJson);
}

bool MainComponent::setMacroValue(const std::string& macroId, double value)
{
    return engineFacade.setMacroValue(macroId, value);
}
} // namespace drs::standalone
