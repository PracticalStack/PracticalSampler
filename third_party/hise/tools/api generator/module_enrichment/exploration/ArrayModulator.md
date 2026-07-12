# Array Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/modulators/mods/ArrayModulator.h`, `hi_core/hi_modules/modulators/mods/ArrayModulator.cpp`
**Base class:** `VoiceStartModulator`, `SliderPackProcessor`

## Signal Path

MIDI noteOn -> extract note number -> index into 128-element SliderPack -> return slider value -> output (modulation signal)

The signal path is minimal. When a voice starts, `calculateVoiceStartValue()` extracts the MIDI note number from the HiseEvent, uses it as a direct index into the SliderPack data array, updates the displayed index (for UI feedback), and returns the slider value at that index. The VoiceStartModulator base class handles storing the result per-voice via `startVoice()` and retrieving it via `getVoiceStartValue()`.

## Gap Answers

### signal-path-order: How does calculateVoiceStartValue() work?

`calculateVoiceStartValue(const HiseEvent &m)` calls `m.getNoteNumber()` to get the MIDI note number (0-127), calls `data->setDisplayedIndex(number)` for UI feedback, then returns `data->getValue(number)`. The note number is used as a direct array index with no bounds checking needed because MIDI note numbers are guaranteed 0-127 and the slider pack has exactly 128 entries. There is no transformation, scaling, or clamping -- the raw slider value is returned directly.

### sliderpack-indexing: How is the SliderPack indexed?

The SliderPack has exactly 128 entries, one per MIDI note number (0-127). This is configured in `referenceShared()` which calls `data->setNumSliders(128)`. The constructor initializes all 128 values to 1.0f via a loop: `for(int i = 0; i < 128; i++) data->setValue(i, 1.0f, dontSendNotification)`. So by default, every note produces full modulation output (1.0). The SliderPackProcessor is constructed with 1 slider pack: `SliderPackProcessor(mc, 1)`.

### output-range: What is the output range?

The range is set in `referenceShared()` via `data->setRange(0.0, 1.0, 0.001)`. This means slider values are constrained to 0.0-1.0 with a step size of 0.001. The SliderPackData range is enforced by the UI and by `setValue()` which respects the configured range. No additional clamping is applied in `calculateVoiceStartValue()` -- it returns the raw `getValue()` result.

### description-accuracy-no-params: Confirm no parameters

Confirmed. The `SpecialParameters` enum contains only `numTotalParameters` (value 0), meaning zero custom parameters. The `getAttribute()` and `setInternalAttribute()` methods delegate directly to `data->getValue(index)` and `data->setValue(index, value)` -- these operate on SliderPack data slots, not on named parameters. The SliderPack IS the entire configuration. There are no hidden toggle or mode parameters.

## Processing Chain Detail

1. **Note number extraction** (per-voice, negligible CPU): Reads the note number from the incoming HiseEvent.
2. **SliderPack lookup** (per-voice, negligible CPU): Direct array index into the 128-element SliderPack. Single memory read.
3. **Display index update** (per-voice, negligible CPU): Sets the displayed slider index for UI highlighting. No audio impact.

## Modulation Points

None. ArrayModulator has no child modulation chains (`getNumChildProcessors()` returns 0, inherited from VoiceStartModulator which declares this as `final`). The Intensity knob inherited from the Modulation base class scales the output, but this is handled by the base class infrastructure, not by ArrayModulator itself.

## Interface Usage

### SliderPackProcessor

The module inherits from `SliderPackProcessor` and constructs it with 1 slider pack. In `referenceShared()`, it calls `getSliderPackUnchecked(0)` to obtain a pointer to the SliderPackData, which is stored as `data`. This pointer is used directly in `calculateVoiceStartValue()` for the lookup and in `getAttribute()`/`setInternalAttribute()` for scripted parameter access.

The SliderPack can be accessed from HiseScript via `Synth.getSliderPackProcessor("modulatorId").getSliderPack(0)`, which returns a ScriptSliderPackData handle for runtime manipulation of individual slider values.

Serialization uses `data->toBase64()` / `data->fromBase64()` stored in the ValueTree under the property key "SliderPackData".

## CPU Assessment

Overall: **negligible**. The entire processing is a single array lookup per voice start. No per-sample processing, no mathematical operations beyond the index read. This is one of the cheapest possible modulators.

## UI Components

The editor is `ArrayModulatorEditor` (defined in `hi_core/hi_modules/modulators/editors/ArrayModulatorEditor.h`). It contains a single `SliderPack` component (`sliderPackMix`) that displays the 128 sliders. No FloatingTile content types are registered -- this uses a traditional ProcessorEditorBody.

## Notes

- The module's class-level Doxygen comment says "This modulator simply returns a constant value that can be used to change the gain or something else" which is copy-pasted from ConstantModulator and inaccurate. The metadata description in the .cpp file is correct: "Creates a modulation signal from a slider pack array indexed by MIDI note number, allowing per-note modulation values."
- Default slider values are all 1.0, meaning the modulator is transparent by default (full modulation output for every note). Users must explicitly lower slider values to create per-note variation.
- The `referenceShared()` method is called in the constructor and supports the HISE shared data reference system, allowing multiple modules to share the same SliderPack data.
