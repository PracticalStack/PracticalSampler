#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace drs::app
{
class DrumPadComponent final : public juce::Component
{
public:
    struct Pad
    {
        int midiNote = 36;
        juce::String name;
        juce::String subtitle;
        juce::Colour colour;
        juce::KeyPress triggerKey;
    };

    using NoteOnCallback = std::function<void(int midiNote, float velocity, int padIndex)>;
    using NoteOffCallback = std::function<void(int midiNote, int padIndex)>;

    DrumPadComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    void setPads(std::vector<Pad> nextPads);
    const std::vector<Pad>& getPads() const noexcept { return pads; }

    void setPadColumns(int newColumnCount);
    int getPadColumns() const noexcept { return columnCount; }

    void setPadSpacing(int newPadSpacing);
    int getPadSpacing() const noexcept { return padSpacing; }

    void setVelocity(float newVelocity);
    float getVelocity() const noexcept { return velocity; }

    void setUseMousePositionForVelocity(bool shouldUseMousePositionForVelocity);
    bool getUseMousePositionForVelocity() const noexcept { return useMousePositionForVelocity; }

    void setNoteOnCallback(NoteOnCallback nextCallback);
    void setNoteOffCallback(NoteOffCallback nextCallback);
    void setCallbacks(NoteOnCallback nextNoteOnCallback, NoteOffCallback nextNoteOffCallback);

    void releaseAllPads();

    static std::vector<Pad> createGeneralMidiLayout();

private:
    void updatePadLayout();
    int getPadIndexAt(juce::Point<float> position) const;
    float resolveVelocityForPosition(int padIndex, juce::Point<float> position) const;
    bool isPadActive(int padIndex) const;
    void setMousePadState(int padIndex, bool shouldBeDown, float noteVelocity = 1.0f);
    void setKeyboardPadState(int padIndex, bool shouldBeDown);
    bool syncKeyboardState();
    juce::Colour getBasePadColour(int padIndex) const;
    juce::String buildKeyHintText(const Pad& pad) const;

    std::vector<Pad> pads;
    std::vector<juce::Rectangle<float>> padBounds;
    std::vector<bool> mousePadStates;
    std::vector<bool> keyboardPadStates;
    int hoveredPadIndex = -1;
    int activeMousePadIndex = -1;
    int columnCount = 4;
    int padSpacing = 10;
    float velocity = 1.0f;
    bool useMousePositionForVelocity = true;
    NoteOnCallback noteOnCallback;
    NoteOffCallback noteOffCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadComponent)
};
} // namespace drs::app
