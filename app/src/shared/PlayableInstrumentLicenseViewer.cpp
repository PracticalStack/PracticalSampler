#include "shared/PlayableInstrumentLicenseViewer.h"

#include <algorithm>

namespace drs::app
{
namespace
{
const auto viewerBackground = juce::Colour::fromRGB(245, 240, 232);
const auto viewerPanel = juce::Colour::fromRGB(255, 252, 246);
const auto viewerInk = juce::Colour::fromRGB(25, 31, 36);
const auto viewerMuted = juce::Colour::fromRGB(96, 103, 112);
const auto viewerAccent = juce::Colour::fromRGB(150, 78, 22);

juce::String toDisplayString(const std::string& text)
{
    return juce::String::fromUTF8(text.data(), static_cast<int>(text.size()));
}
} // namespace

PlayableInstrumentLicenseViewer::PlayableInstrumentLicenseViewer(
    std::shared_ptr<const std::string> licenseText)
{
    setOpaque(true);
    setComponentID("playableInstrumentLicenseViewer");
    setTitle("Playable instrument license");
    setDescription("Read-only license text included by the playable instrument author.");

    headingLabel.setComponentID("playableInstrumentLicenseHeading");
    headingLabel.setText("Instrument license", juce::dontSendNotification);
    headingLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    headingLabel.setColour(juce::Label::textColourId, viewerInk);
    headingLabel.setJustificationType(juce::Justification::centredLeft);
    headingLabel.setTitle("Instrument license");
    addAndMakeVisible(headingLabel);

    guidanceLabel.setComponentID("playableInstrumentLicenseGuidance");
    guidanceLabel.setText("This text is included with the loaded playable instrument.",
                          juce::dontSendNotification);
    guidanceLabel.setFont(juce::FontOptions(14.0f));
    guidanceLabel.setColour(juce::Label::textColourId, viewerMuted);
    guidanceLabel.setJustificationType(juce::Justification::centredLeft);
    guidanceLabel.setDescription("The license belongs to the currently loaded playable instrument.");
    addAndMakeVisible(guidanceLabel);

    licenseEditor.setComponentID("playableInstrumentLicenseText");
    licenseEditor.setTitle("License text");
    licenseEditor.setDescription("Selectable read-only license text.");
    licenseEditor.setMultiLine(true, true);
    licenseEditor.setReturnKeyStartsNewLine(false);
    licenseEditor.setReadOnly(true);
    licenseEditor.setScrollbarsShown(true);
    licenseEditor.setPopupMenuEnabled(true);
    licenseEditor.setCaretVisible(true);
    licenseEditor.setFont(juce::FontOptions(14.0f));
    licenseEditor.setColour(juce::TextEditor::backgroundColourId, viewerPanel);
    licenseEditor.setColour(juce::TextEditor::textColourId, viewerInk);
    licenseEditor.setColour(juce::TextEditor::outlineColourId, viewerMuted.withAlpha(0.45f));
    licenseEditor.setText(licenseText != nullptr ? toDisplayString(*licenseText) : juce::String(), false);
    addAndMakeVisible(licenseEditor);

    closeButton.setComponentID("playableInstrumentLicenseCloseButton");
    closeButton.setTitle("Close license viewer");
    closeButton.setDescription("Closes the playable instrument license viewer.");
    closeButton.setColour(juce::TextButton::buttonColourId, viewerAccent);
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeButton.onClick = [this] { closeDialog(); };
    addAndMakeVisible(closeButton);

    setSize(720, 540);
}

void PlayableInstrumentLicenseViewer::paint(juce::Graphics& graphics)
{
    graphics.fillAll(viewerBackground);
}

void PlayableInstrumentLicenseViewer::resized()
{
    auto area = getLocalBounds().reduced(18);
    const auto buttonHeight = std::min(34, std::max(24, area.getHeight() / 7));
    auto footer = area.removeFromBottom(buttonHeight);
    closeButton.setBounds(footer.removeFromRight(std::min(112, footer.getWidth())));

    area.removeFromBottom(12);
    headingLabel.setBounds(area.removeFromTop(std::min(34, area.getHeight())));
    guidanceLabel.setBounds(area.removeFromTop(std::min(28, area.getHeight())));
    area.removeFromTop(8);
    licenseEditor.setBounds(area);
}

void PlayableInstrumentLicenseViewer::closeDialog()
{
    if (auto* dialogWindow = findParentComponentOfClass<juce::DialogWindow>())
        dialogWindow->exitModalState(0);
}

void showPlayableInstrumentLicenseViewerDialog(
    juce::Component* parentComponent,
    std::shared_ptr<const std::string> licenseText)
{
    if (licenseText == nullptr)
        return;

    auto* content = new PlayableInstrumentLicenseViewer(std::move(licenseText));
    if (parentComponent != nullptr)
    {
        const auto width = std::clamp(parentComponent->getWidth() - 24, 240, 760);
        const auto height = std::clamp(parentComponent->getHeight() - 24, 220, 620);
        content->setSize(width, height);
    }

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "License";
    options.dialogBackgroundColour = viewerBackground;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.useBottomRightCornerResizer = false;
    options.content.setOwned(content);
    options.componentToCentreAround = parentComponent;
    options.launchAsync();
}
} // namespace drs::app
