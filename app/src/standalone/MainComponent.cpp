#include "standalone/MainComponent.h"

namespace drs::standalone
{
MainComponent::MainComponent()
    : statusPanel(engineFacade)
{
    addAndMakeVisible(statusPanel);
    setSize(820, 520);
}

void MainComponent::resized()
{
    statusPanel.setBounds(getLocalBounds());
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
