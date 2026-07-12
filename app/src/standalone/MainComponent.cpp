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
} // namespace drs::standalone
