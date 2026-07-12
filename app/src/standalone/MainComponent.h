#pragma once

#include "drs/engine/EngineFacade.h"
#include "shared/StatusPanel.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace drs::standalone
{
class MainComponent final : public juce::Component
{
public:
    MainComponent();

    void resized() override;

private:
    drs::engine::EngineFacade engineFacade;
    drs::app::StatusPanel statusPanel;
};
} // namespace drs::standalone
