#include "shared/midi/DrumPadComponent.h"

#include <algorithm>
#include <array>

namespace drs::app
{
namespace
{
const auto drumPadBackground = juce::Colour::fromRGB(21, 25, 30);
const auto drumPadEmptyState = juce::Colour::fromRGB(148, 154, 163);
const auto drumPadOutline = juce::Colour::fromRGB(64, 72, 81);
const auto drumPadText = juce::Colours::white;
const auto drumPadSubtext = juce::Colour::fromRGB(214, 221, 226);
const auto drumPadBadge = juce::Colour::fromRGBA(255, 255, 255, 48);
const auto drumPadFocus = juce::Colour::fromRGB(255, 244, 220);
const auto drumPadFocusHalo = juce::Colour::fromRGBA(255, 255, 255, 188);

const std::array<juce::Colour, 8> defaultPadPalette
{
    juce::Colour::fromRGB(170, 88, 45),
    juce::Colour::fromRGB(182, 110, 32),
    juce::Colour::fromRGB(161, 72, 62),
    juce::Colour::fromRGB(136, 88, 31),
    juce::Colour::fromRGB(69, 118, 112),
    juce::Colour::fromRGB(83, 104, 156),
    juce::Colour::fromRGB(131, 86, 148),
    juce::Colour::fromRGB(121, 106, 54)
};

void drawFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerSize)
{
    g.setColour(drumPadFocusHalo);
    g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, 3.0f);
    g.setColour(drumPadFocus);
    g.drawRoundedRectangle(bounds, cornerSize, 1.8f);
}

void configureAccessibleMetadata(juce::Component& component,
                                 const juce::String& title,
                                 const juce::String& description,
                                 const juce::String& helpText = {})
{
    component.setTitle(title);
    component.setDescription(description);

    if (helpText.isNotEmpty())
        component.setHelpText(helpText);
}
} // namespace

DrumPadComponent::DrumPadComponent()
{
    setComponentID("drumPadComponent");
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    configureAccessibleMetadata(*this,
                                "Drum pad grid",
                                "A grid of playable MIDI drum pads.",
                                "Click a pad to trigger it, or use the mapped keyboard shortcut shown on each pad.");
}

void DrumPadComponent::paint(juce::Graphics& g)
{
    g.fillAll(drumPadBackground);

    if (pads.empty())
    {
        g.setColour(drumPadEmptyState);
        g.setFont(juce::FontOptions(15.0f));
        g.drawFittedText("Add pads with setPads() to play drum notes.",
                         getLocalBounds().reduced(16),
                         juce::Justification::centred,
                         2);
        return;
    }

    for (std::size_t index = 0; index < pads.size(); ++index)
    {
        auto bounds = padBounds[index].reduced(1.0f);
        const auto active = isPadActive(static_cast<int>(index));
        const auto hovered = static_cast<int>(index) == hoveredPadIndex;
        auto fill = getBasePadColour(static_cast<int>(index));

        if (active)
            fill = fill.brighter(0.28f);
        else if (hovered)
            fill = fill.brighter(0.12f);

        if (hasKeyboardFocus(true) && static_cast<int>(index) == hoveredPadIndex)
            drawFocusRing(g, bounds.reduced(1.0f), 13.0f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 12.0f);

        g.setColour(active ? juce::Colours::white : drumPadOutline.withAlpha(0.9f));
        g.drawRoundedRectangle(bounds, 12.0f, active ? 1.5f : 1.0f);

        auto content = bounds.toNearestInt().reduced(12, 10);
        auto topRow = content.removeFromTop(18);
        auto badgeArea = topRow.removeFromRight(44);

        g.setColour(drumPadBadge);
        g.fillRoundedRectangle(badgeArea.toFloat(), 8.0f);
        g.setColour(drumPadText.withAlpha(0.9f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText(juce::String(pads[index].midiNote),
                         badgeArea,
                         juce::Justification::centred,
                         1);

        g.setColour(drumPadText);
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawFittedText(pads[index].name.isNotEmpty() ? pads[index].name : "Pad " + juce::String(static_cast<int>(index) + 1),
                         content.removeFromTop(24),
                         juce::Justification::topLeft,
                         1);

        if (pads[index].subtitle.isNotEmpty())
        {
            g.setColour(drumPadSubtext);
            g.setFont(juce::FontOptions(12.0f));
            g.drawFittedText(pads[index].subtitle,
                             content.removeFromTop(18),
                             juce::Justification::topLeft,
                             1);
        }

        const auto keyHint = buildKeyHintText(pads[index]);
        if (keyHint.isNotEmpty())
        {
            auto hintArea = bounds.toNearestInt().removeFromBottom(24).reduced(12, 4);
            g.setColour(drumPadText.withAlpha(0.82f));
            g.setFont(juce::FontOptions(11.0f));
            g.drawFittedText(keyHint, hintArea, juce::Justification::centredLeft, 1);
        }
    }
}

void DrumPadComponent::resized()
{
    updatePadLayout();
}

void DrumPadComponent::mouseMove(const juce::MouseEvent& event)
{
    hoveredPadIndex = getPadIndexAt(event.position);
    repaint();
}

void DrumPadComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredPadIndex = -1;
    repaint();
}

void DrumPadComponent::mouseDown(const juce::MouseEvent& event)
{
    hoveredPadIndex = getPadIndexAt(event.position);
    activeMousePadIndex = hoveredPadIndex;

    if (activeMousePadIndex >= 0)
        setMousePadState(activeMousePadIndex,
                         true,
                         resolveVelocityForPosition(activeMousePadIndex, event.position));

    repaint();
}

void DrumPadComponent::mouseDrag(const juce::MouseEvent& event)
{
    const auto nextPadIndex = getPadIndexAt(event.position);
    hoveredPadIndex = nextPadIndex;

    if (nextPadIndex != activeMousePadIndex)
    {
        if (activeMousePadIndex >= 0)
            setMousePadState(activeMousePadIndex, false);

        activeMousePadIndex = nextPadIndex;

        if (activeMousePadIndex >= 0)
            setMousePadState(activeMousePadIndex,
                             true,
                             resolveVelocityForPosition(activeMousePadIndex, event.position));
    }

    repaint();
}

void DrumPadComponent::mouseUp(const juce::MouseEvent& event)
{
    hoveredPadIndex = getPadIndexAt(event.position);

    if (activeMousePadIndex >= 0)
        setMousePadState(activeMousePadIndex, false);

    activeMousePadIndex = -1;
    repaint();
}

bool DrumPadComponent::keyPressed(const juce::KeyPress& key)
{
    for (std::size_t index = 0; index < pads.size(); ++index)
    {
        if (pads[index].triggerKey == key)
        {
            hoveredPadIndex = static_cast<int>(index);
            setKeyboardPadState(static_cast<int>(index), true);
            repaint();
            return true;
        }
    }

    return juce::Component::keyPressed(key);
}

bool DrumPadComponent::keyStateChanged(bool isKeyDown)
{
    const auto changed = syncKeyboardState();
    if (changed)
        repaint();

    return changed || juce::Component::keyStateChanged(isKeyDown);
}

void DrumPadComponent::focusGained(FocusChangeType cause)
{
    juce::Component::focusGained(cause);
    repaint();
}

void DrumPadComponent::focusLost(FocusChangeType cause)
{
    juce::Component::focusLost(cause);
    hoveredPadIndex = -1;
    syncKeyboardState();
    repaint();
}

void DrumPadComponent::setPads(std::vector<Pad> nextPads)
{
    releaseAllPads();

    pads = std::move(nextPads);
    padBounds.assign(pads.size(), {});
    mousePadStates.assign(pads.size(), false);
    keyboardPadStates.assign(pads.size(), false);
    hoveredPadIndex = -1;
    activeMousePadIndex = -1;
    updatePadLayout();

    auto description = juce::String("A grid of ")
        + juce::String(static_cast<int>(pads.size()))
        + " playable MIDI drum pads.";
    if (!pads.empty())
        description += " Use the keyboard shortcuts shown on each pad for quick auditioning.";
    setDescription(description);

    repaint();
}

void DrumPadComponent::setPadColumns(int newColumnCount)
{
    columnCount = juce::jmax(1, newColumnCount);
    updatePadLayout();
    repaint();
}

void DrumPadComponent::setPadSpacing(int newPadSpacing)
{
    padSpacing = juce::jmax(0, newPadSpacing);
    updatePadLayout();
    repaint();
}

void DrumPadComponent::setVelocity(float newVelocity)
{
    velocity = juce::jlimit(0.0f, 1.0f, newVelocity);
}

void DrumPadComponent::setUseMousePositionForVelocity(bool shouldUseMousePositionForVelocity)
{
    useMousePositionForVelocity = shouldUseMousePositionForVelocity;
}

void DrumPadComponent::setNoteOnCallback(NoteOnCallback nextCallback)
{
    noteOnCallback = std::move(nextCallback);
}

void DrumPadComponent::setNoteOffCallback(NoteOffCallback nextCallback)
{
    noteOffCallback = std::move(nextCallback);
}

void DrumPadComponent::setCallbacks(NoteOnCallback nextNoteOnCallback, NoteOffCallback nextNoteOffCallback)
{
    noteOnCallback = std::move(nextNoteOnCallback);
    noteOffCallback = std::move(nextNoteOffCallback);
}

void DrumPadComponent::releaseAllPads()
{
    for (std::size_t index = 0; index < pads.size(); ++index)
    {
        if (isPadActive(static_cast<int>(index)) && noteOffCallback)
            noteOffCallback(pads[index].midiNote, static_cast<int>(index));
    }

    std::fill(mousePadStates.begin(), mousePadStates.end(), false);
    std::fill(keyboardPadStates.begin(), keyboardPadStates.end(), false);
    activeMousePadIndex = -1;
    repaint();
}

std::vector<DrumPadComponent::Pad> DrumPadComponent::createGeneralMidiLayout()
{
    return {
        { 36, "Kick", "C1", defaultPadPalette[0], juce::KeyPress('q') },
        { 38, "Snare", "D1", defaultPadPalette[1], juce::KeyPress('w') },
        { 42, "Closed Hat", "F#1", defaultPadPalette[2], juce::KeyPress('e') },
        { 46, "Open Hat", "A#1", defaultPadPalette[3], juce::KeyPress('r') },
        { 39, "Clap", "D#1", defaultPadPalette[4], juce::KeyPress('a') },
        { 41, "Low Tom", "F1", defaultPadPalette[5], juce::KeyPress('s') },
        { 45, "Mid Tom", "A1", defaultPadPalette[6], juce::KeyPress('d') },
        { 48, "High Tom", "C2", defaultPadPalette[7], juce::KeyPress('f') },
        { 49, "Crash", "C#2", defaultPadPalette[0], juce::KeyPress('z') },
        { 51, "Ride", "D#2", defaultPadPalette[1], juce::KeyPress('x') },
        { 37, "Side Stick", "C#1", defaultPadPalette[2], juce::KeyPress('c') },
        { 82, "Shaker", "A#5", defaultPadPalette[3], juce::KeyPress('v') }
    };
}

void DrumPadComponent::updatePadLayout()
{
    if (pads.empty())
    {
        padBounds.clear();
        return;
    }

    const auto bounds = getLocalBounds().reduced(4).toFloat();
    const auto columns = juce::jmax(1, juce::jmin(columnCount, static_cast<int>(pads.size())));
    const auto rows = juce::jmax(1, (static_cast<int>(pads.size()) + columns - 1) / columns);
    const auto horizontalSpacing = static_cast<float>(padSpacing);
    const auto verticalSpacing = static_cast<float>(padSpacing);
    const auto padWidth = juce::jmax(24.0f,
                                     (bounds.getWidth() - horizontalSpacing * static_cast<float>(columns - 1))
                                         / static_cast<float>(columns));
    const auto padHeight = juce::jmax(24.0f,
                                      (bounds.getHeight() - verticalSpacing * static_cast<float>(rows - 1))
                                          / static_cast<float>(rows));

    padBounds.resize(pads.size());
    for (std::size_t index = 0; index < pads.size(); ++index)
    {
        const auto row = static_cast<int>(index) / columns;
        const auto column = static_cast<int>(index) % columns;
        const auto x = bounds.getX() + static_cast<float>(column) * (padWidth + horizontalSpacing);
        const auto y = bounds.getY() + static_cast<float>(row) * (padHeight + verticalSpacing);
        padBounds[index] = { x, y, padWidth, padHeight };
    }
}

int DrumPadComponent::getPadIndexAt(juce::Point<float> position) const
{
    for (std::size_t index = 0; index < padBounds.size(); ++index)
    {
        if (padBounds[index].contains(position))
            return static_cast<int>(index);
    }

    return -1;
}

float DrumPadComponent::resolveVelocityForPosition(int padIndex, juce::Point<float> position) const
{
    if (!useMousePositionForVelocity
        || padIndex < 0
        || static_cast<std::size_t>(padIndex) >= padBounds.size())
    {
        return velocity;
    }

    const auto bounds = padBounds[static_cast<std::size_t>(padIndex)];
    if (bounds.getHeight() <= 0.0f)
        return velocity;

    const auto normalized = juce::jlimit(0.0f, 1.0f, 1.0f - ((position.y - bounds.getY()) / bounds.getHeight()));
    return juce::jlimit(0.0f, 1.0f, normalized);
}

bool DrumPadComponent::isPadActive(int padIndex) const
{
    if (padIndex < 0 || static_cast<std::size_t>(padIndex) >= pads.size())
        return false;

    return mousePadStates[static_cast<std::size_t>(padIndex)]
        || keyboardPadStates[static_cast<std::size_t>(padIndex)];
}

void DrumPadComponent::setMousePadState(int padIndex, bool shouldBeDown, float noteVelocity)
{
    if (padIndex < 0 || static_cast<std::size_t>(padIndex) >= pads.size())
        return;

    const auto index = static_cast<std::size_t>(padIndex);
    const auto wasActive = isPadActive(padIndex);
    mousePadStates[index] = shouldBeDown;
    const auto isActiveNow = isPadActive(padIndex);

    if (!wasActive && isActiveNow)
    {
        if (noteOnCallback)
            noteOnCallback(pads[index].midiNote, juce::jlimit(0.0f, 1.0f, noteVelocity), padIndex);
    }
    else if (wasActive && !isActiveNow)
    {
        if (noteOffCallback)
            noteOffCallback(pads[index].midiNote, padIndex);
    }
}

void DrumPadComponent::setKeyboardPadState(int padIndex, bool shouldBeDown)
{
    if (padIndex < 0 || static_cast<std::size_t>(padIndex) >= pads.size())
        return;

    const auto index = static_cast<std::size_t>(padIndex);
    const auto wasActive = isPadActive(padIndex);
    keyboardPadStates[index] = shouldBeDown;
    const auto isActiveNow = isPadActive(padIndex);

    if (!wasActive && isActiveNow)
    {
        if (noteOnCallback)
            noteOnCallback(pads[index].midiNote, velocity, padIndex);
    }
    else if (wasActive && !isActiveNow)
    {
        if (noteOffCallback)
            noteOffCallback(pads[index].midiNote, padIndex);
    }
}

bool DrumPadComponent::syncKeyboardState()
{
    bool changed = false;

    for (std::size_t index = 0; index < pads.size(); ++index)
    {
        const auto shouldBeDown = pads[index].triggerKey.isValid() && pads[index].triggerKey.isCurrentlyDown();
        if (keyboardPadStates[index] != shouldBeDown)
        {
            setKeyboardPadState(static_cast<int>(index), shouldBeDown);
            changed = true;
        }
    }

    return changed;
}

juce::Colour DrumPadComponent::getBasePadColour(int padIndex) const
{
    if (padIndex < 0 || static_cast<std::size_t>(padIndex) >= pads.size())
        return defaultPadPalette.front();

    const auto& pad = pads[static_cast<std::size_t>(padIndex)];
    if (pad.colour.isTransparent())
        return defaultPadPalette[static_cast<std::size_t>(padIndex) % defaultPadPalette.size()];

    return pad.colour;
}

juce::String DrumPadComponent::buildKeyHintText(const Pad& pad) const
{
    if (!pad.triggerKey.isValid())
        return {};

    return "Key: " + pad.triggerKey.getTextDescription().toUpperCase();
}
} // namespace drs::app
