#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"
#include "shared/authoring/WaveformDetailView.h"
#include "shared/authoring/ZoneMapCanvas.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

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

double linearChannel(const float channel)
{
    return channel <= 0.04045f
        ? channel / 12.92
        : std::pow((channel + 0.055) / 1.055, 2.4);
}

double luminance(const juce::Colour colour)
{
    return 0.2126 * linearChannel(colour.getFloatRed())
        + 0.7152 * linearChannel(colour.getFloatGreen())
        + 0.0722 * linearChannel(colour.getFloatBlue());
}

double contrast(const juce::Colour first, const juce::Colour second)
{
    const auto light = std::max(luminance(first), luminance(second));
    const auto dark = std::min(luminance(first), luminance(second));
    return (light + 0.05) / (dark + 0.05);
}

void qualifyVisualTokens()
{
    using namespace drs::app::authoring::visual;
    require(contrast(text, surface) >= 7.0,
            "Primary graphite text must retain enhanced contrast on the work surface.");
    require(contrast(textMuted, surface) >= 4.5,
            "Secondary metadata must retain AA contrast on the work surface.");
    require(contrast(textOnAccent, selection) >= 4.5,
            "Text on an authored orange selection must retain AA contrast.");
    require(contrast(focus, surface) >= 4.5,
            "The blue keyboard focus ring must remain visible on the work surface.");
    require(selection != focus && selection != modulation && focus != information,
            "Selection, keyboard focus, modulation, and information need independent semantics.");
    require(controlRadius <= 2.0f && panelRadius <= 3.0f && borderWidth == 1.0f,
            "Open Workbench geometry must remain square and divider-led.");
    require(compactRowHeight == 24 && controlHeight == 28 && toolbarHeight == 28,
            "Dense desktop metrics changed outside the shared token source.");
    require(titleTypeSize == 24.0f && sectionTypeSize == 16.0f
                && bodyTypeSize == 13.0f && metadataTypeSize == 11.0f,
            "Desktop type roles changed outside the shared token source.");
    require(stableGroupTint("strings") == stableGroupTint("strings")
                && stableGroupTint("strings") != stableGroupTint("brass"),
            "Map group tints must be stable while retaining useful variation.");
}

void qualifyAuthoringSurfaceAndStates()
{
    using namespace drs::app::authoring;
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 4 qualification requires the reference project.");
    drs::engine::AuthoringSession session(loaded.project);
    drs::app::AuthoringPanel panel(session, {}, {}, {}, drs::app::AuthoringPanel::LayoutMode::expanded);
    panel.setSize(1120, 800);
    panel.setVisible(true);
    panel.resized();
    panel.reloadFromSession();

    juce::Image shellImage(juce::Image::ARGB, panel.getWidth(), panel.getHeight(), true);
    {
        juce::Graphics shellGraphics(shellImage);
        panel.paint(shellGraphics);
    }
    require(shellImage.getPixelAt(2, 2) == visual::shell,
            "Authoring panel must paint the shared light shell at its edge.");
    require(shellImage.getPixelAt(30, 30) == visual::surface,
            "Authoring panel must use a warm work surface instead of the former dark card.");

    auto* summaryTitle = dynamic_cast<juce::Label*>(findDescendantById(panel, "authoringSummaryTitleLabel"));
    require(summaryTitle != nullptr
                && summaryTitle->findColour(juce::Label::textColourId) == visual::text,
            "The authoring summary title must no longer depend on a dark background.");

    auto* tab = dynamic_cast<juce::Button*>(findDescendantById(panel, "authoringDrawerWaveformTab"));
    require(tab != nullptr,
            "Phase 4 qualification could not locate the shared workbench tabs.");
    require(tab->findColour(juce::TextButton::buttonColourId) == visual::surfaceRaised
                && tab->findColour(juce::TextButton::buttonOnColourId) == visual::selection,
            "Workbench tabs must use neutral idle and orange selected states.");
    require(tab->findColour(juce::TextEditor::focusedOutlineColourId) == visual::focus,
            "Keyboard focus must be blue and independent of orange selection.");

    auto* map = dynamic_cast<ZoneMapCanvas*>(findDescendantById(panel, "authoringZoneMap"));
    require(map != nullptr
                && map->findColour(juce::ListBox::backgroundColourId) == visual::mapSurface
                && map->findColour(juce::TextEditor::focusedOutlineColourId) == visual::focus,
            "Zone Map surface and focus state must be supplied by semantic tokens.");
}

void qualifyCustomPaintSurfaces()
{
    using namespace drs::app::authoring;
    WaveformDetailView waveform;
    waveform.setSize(320, 120);
    juce::Image waveformImage(juce::Image::ARGB, 320, 120, true);
    {
        juce::Graphics waveformGraphics(waveformImage);
        waveform.paint(waveformGraphics);
    }
    require(waveformImage.getPixelAt(12, 12) == visual::mapSurface,
            "Waveform must share the light data surface.");

    ZoneMapCanvas map;
    map.setSize(640, 360);
    juce::Image mapImage(juce::Image::ARGB, 640, 360, true);
    {
        juce::Graphics mapGraphics(mapImage);
        map.paint(mapGraphics);
    }
    require(mapImage.getPixelAt(12, 12) == visual::surfaceSubtle,
            "Zone Map toolbar must use the shared recessed surface.");
    require(mapImage.getPixelAt(320, 200) != juce::Colour::fromRGB(18, 24, 29),
            "Zone Map custom painting must not reintroduce the old dark shell.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyVisualTokens();
        qualifyAuthoringSurfaceAndStates();
        qualifyCustomPaintSurfaces();
        std::cout << "Open Workbench Phase 4 tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 4 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
