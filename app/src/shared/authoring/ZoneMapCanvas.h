#pragma once

#include "drs/engine/AuthoringSession.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace drs::app::authoring
{
class ZoneMapCanvas final : public juce::Component
{
public:
    void setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries);
    void paint(juce::Graphics& g) override;

private:
    std::vector<drs::engine::AuthoringZoneSummary> zoneSummaries;
};
} // namespace drs::app::authoring
