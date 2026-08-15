#include "drs/engine/EngineFacade.h"
#include "shared/PerformanceMixer.h"
#include "shared/PerformancePanel.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto* child : root.getChildren())
        if (auto* match = findDescendantById(*child, id); match != nullptr)
            return match;
    return nullptr;
}

juce::Label& requireLabel(juce::Component& root, const juce::String& id)
{
    auto* label = dynamic_cast<juce::Label*>(findDescendantById(root, id));
    require(label != nullptr, "Missing Phase 6 label: " + id.toStdString());
    return *label;
}

juce::Button& requireButton(juce::Component& root, const juce::String& id)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, id));
    require(button != nullptr, "Missing Phase 6 button: " + id.toStdString());
    return *button;
}

void requireInside(const juce::Rectangle<int>& outer,
                   const juce::Rectangle<int>& inner,
                   const std::string& message)
{
    require(!inner.isEmpty() && outer.contains(inner), message);
}

std::vector<drs::app::PerformanceMixerControlView> makeMixerControls()
{
    std::vector<drs::app::PerformanceMixerControlView> controls;
    for (int index = 0; index < 3; ++index)
    {
        drs::app::PerformanceMixerControlView control;
        control.authoredId = "phase6-authored-" + std::to_string(index);
        control.runtimeId = "phase6-runtime-" + std::to_string(index);
        control.sectionLabel = "Performance";
        control.controlLabel = "Control " + std::to_string(index + 1);
        control.valueUnit = "%";
        control.accessibilityDescription = "Phase 6 published control";
        control.authoredOrder = static_cast<std::size_t>(index);
        control.value = 0.25 * static_cast<double>(index + 1);
        controls.push_back(std::move(control));
    }
    return controls;
}

void qualifyExpandedAndCompactLayouts(drs::engine::EngineFacade& facade)
{
    juce::String workspaceDisplayName { "Phase 6 Project" };
    drs::app::PerformancePanel panel(
        facade, {}, {}, {}, {}, {}, {}, {}, {},
        [&workspaceDisplayName] { return workspaceDisplayName; });
    panel.setSize(1120, 800);
    panel.setVisible(true);
    panel.refreshNow();

    const auto initial = panel.getLayoutSnapshot();
    const auto panelBounds = panel.getLocalBounds();
    require(!initial.compact && !initial.shortHeight,
            "The target Perform surface must use the expanded layout policy.");
    requireInside(panelBounds, initial.headerBounds,
                  "The Perform identity header must remain inside the target shell.");
    requireInside(panelBounds, initial.controlsBounds,
                  "The collapsed control strip must remain inside the target shell.");
    requireInside(panelBounds, initial.artworkBounds,
                  "Supporting artwork must retain a bounded target region.");
    requireInside(panelBounds, initial.keyboardBounds,
                  "The performance keyboard must retain a bounded target region.");
    require(initial.keyboardBounds.getY() > initial.artworkBounds.getY(),
            "The keyboard must remain anchored beneath the primary Perform content.");

    auto& instrumentName = requireLabel(panel, "performanceInstrumentNameLabel");
    auto& instrumentContext = requireLabel(panel, "performanceInstrumentContextLabel");
    auto& status = requireLabel(panel, "performanceLoadIndicatorLabel");
    require(instrumentName.getText() == workspaceDisplayName
                && instrumentName.getTitle() == "Performance instrument",
            "Perform must expose the current workspace name as its accessible identity.");
    require(instrumentContext.getText().isNotEmpty()
                && instrumentContext.getTitle() == "Performance context",
            "Perform must expose concise instrument context beneath its identity.");
    require(status.findColour(juce::Label::backgroundColourId)
                == drs::app::authoring::visual::success.withAlpha(0.13f),
            "A loaded Perform fixture must use the shared healthy-state treatment.");

    auto& controlsToggle = requireButton(panel, "performanceMacroStripToggleButton");
    require(controlsToggle.getExplicitFocusOrder() == 20,
            "Instrument Controls disclosure must retain a stable keyboard position.");
    controlsToggle.onClick();
    const auto expanded = panel.getLayoutSnapshot();
    require(expanded.controlsBesideArtwork,
            "Expanded target shells must place primary controls beside supporting artwork.");
    require(expanded.controlsBounds.getRight() < expanded.artworkBounds.getX()
                && expanded.controlsBounds.getWidth() > expanded.artworkBounds.getWidth(),
            "Wide Perform layouts must allocate priority width to Instrument Controls.");

    auto& details = requireButton(panel, "performanceDetailsToggleButton");
    require(details.getButtonText() == "Details"
                && details.getExplicitFocusOrder() == 10
                && details.getDescription().isNotEmpty(),
            "Performance Details must be an explicit accessible disclosure before controls.");
    details.onClick();
    const auto withDetails = panel.getLayoutSnapshot();
    auto* diagnosticsViewport = dynamic_cast<juce::Viewport*>(
        findDescendantById(panel, "performanceDiagnosticsViewport"));
    auto* diagnosticsPanel = findDescendantById(panel, "performanceDiagnosticsPanel");
    require(withDetails.diagnosticsVisible && !withDetails.diagnosticsBounds.isEmpty()
                && details.getButtonText() == "Hide Details"
                && diagnosticsViewport != nullptr && diagnosticsViewport->isVisible()
                && diagnosticsPanel != nullptr
                && diagnosticsPanel->getHeight() > diagnosticsViewport->getHeight(),
            "The Details action must reveal bounded, vertically scroll-safe diagnostics.");
    details.onClick();
    require(!panel.getLayoutSnapshot().diagnosticsVisible
                && details.getButtonText() == "Details",
            "Performance Details must collapse back to the primary player surface.");

    panel.setSize(760, 620);
    panel.resized();
    const auto compact = panel.getLayoutSnapshot();
    require(compact.compact && compact.shortHeight && !compact.controlsBesideArtwork,
            "The minimum plug-in surface must use the compact stacked layout policy.");
    require(compact.controlsBounds.getBottom() <= compact.artworkBounds.getY()
                && compact.artworkBounds.getBottom() < compact.keyboardBounds.getY(),
            "Compact Perform regions must stack without overlap above the anchored keyboard.");
    requireInside(panel.getLocalBounds(), compact.headerBounds,
                  "The compact identity header must remain reachable.");
    requireInside(panel.getLocalBounds(), compact.controlsBounds,
                  "The compact Instrument Controls region must remain reachable.");
    requireInside(panel.getLocalBounds(), compact.keyboardBounds,
                  "The compact keyboard must remain reachable.");

    juce::Image image(juce::Image::ARGB, panel.getWidth(), panel.getHeight(), true);
    {
        juce::Graphics graphics(image);
        panel.paint(graphics);
    }
    const auto shellPixel = image.getPixelAt(0, 0);
    require(shellPixel.getRed() == drs::app::authoring::visual::shell.getRed()
                && shellPixel.getGreen() == drs::app::authoring::visual::shell.getGreen()
                && shellPixel.getBlue() == drs::app::authoring::visual::shell.getBlue(),
            "Perform must paint the shared light application shell at its perimeter (actual "
                + shellPixel.toDisplayString(false).toStdString() + ").");

    workspaceDisplayName.clear();
    panel.refreshNow();
    require(instrumentName.getText().isEmpty()
                && instrumentName.getDescription().contains("No performance instrument or project"),
            "Perform must leave its identity header empty when no workspace is loaded.");
}

void qualifyAudioUnavailableState(drs::engine::EngineFacade& facade)
{
    drs::app::PerformancePanel panel(
        facade, {}, {}, {}, {}, {}, [] { return false; });
    panel.setSize(760, 620);
    panel.refreshNow();

    auto& status = requireLabel(panel, "performanceLoadIndicatorLabel");
    require(status.getText() == "Audio Inactive"
                && status.findColour(juce::Label::backgroundColourId)
                    == drs::app::authoring::visual::warning.withAlpha(0.13f),
            "Audio-unavailable Perform must use the explicit shared warning state.");
    auto* keyboard = findDescendantById(panel, "performanceKeyboard");
    require(keyboard != nullptr && !keyboard->isEnabled()
                && keyboard->getDescription().contains("Audio inactive"),
            "The keyboard must visibly and accessibly explain unavailable audio.");
    require(requireLabel(panel, "performanceGuidanceLabel").getText().contains("audio settings"),
            "Audio-unavailable guidance must expose the existing recovery action.");
}

void qualifyMixerValueRefreshStability()
{
    drs::app::PerformanceMixer mixer;
    mixer.setSize(860, 360);
    auto controls = makeMixerControls();
    mixer.setControls(controls);
    const auto rebuildGeneration = mixer.getRebuildGeneration();
    const auto layout = mixer.getLayoutSnapshot();

    controls[1].value = 0.91;
    mixer.updateControlValues(controls);
    require(mixer.getRebuildGeneration() == rebuildGeneration
                && mixer.getLayoutSnapshot().columnCount == layout.columnCount
                && mixer.getLayoutSnapshot().rowCount == layout.rowCount,
            "Value-only Perform refreshes must not rebuild published controls or layout.");

    auto* lane = findDescendantById(mixer, "performanceMixerControl.phase6-authored-1");
    auto* widget = findDescendantById(mixer, "performanceMixerWidget.phase6-authored-1");
    require(lane != nullptr && widget != nullptr
                && lane->getBounds().getWidth() > 0
                && widget->getExplicitFocusOrder() == 201,
            "Published controls must retain bounded lanes and deterministic focus order.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        drs::engine::EngineFacade facade;
        facade.resetSessionStateToDefault();
        require(facade.waitForPreparedPlaybackIdle(),
                "Phase 6 fixture must finish preparing its default performance.");
        facade.serviceBackgroundWork();

        qualifyExpandedAndCompactLayouts(facade);
        qualifyAudioUnavailableState(facade);
        qualifyMixerValueRefreshStability();

        std::cout << "Open Workbench Phase 6 Perform qualification passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 6 Perform qualification failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
