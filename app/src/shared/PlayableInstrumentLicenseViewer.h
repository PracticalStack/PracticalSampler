#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>
#include <string>

namespace drs::app
{
class PlayableInstrumentLicenseViewer final : public juce::Component
{
public:
    explicit PlayableInstrumentLicenseViewer(std::shared_ptr<const std::string> licenseText);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void closeDialog();

    juce::Label headingLabel;
    juce::Label guidanceLabel;
    juce::TextEditor licenseEditor;
    juce::TextButton closeButton { "Close" };
};

void showPlayableInstrumentLicenseViewerDialog(
    juce::Component* parentComponent,
    std::shared_ptr<const std::string> licenseText);
} // namespace drs::app
