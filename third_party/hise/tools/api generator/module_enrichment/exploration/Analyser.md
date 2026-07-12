# Analyser - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/Analyser.h`, `Analyser.cpp`
**Base class:** `MasterEffectProcessor`, `ProcessorWithStaticExternalData`

## Signal Path

Audio passes through completely unchanged. The only action in `applyEffect()` is writing the audio buffer into a ring buffer for UI display. The module is a pure passthrough with a side-channel data tap.

audio in -> [write to ring buffer if active] -> audio out (unchanged)

## Gap Answers

### passthrough-confirmation

**Question:** Does applyEffect() pass audio through unchanged?

**Answer:** Yes. `applyEffect()` (Analyser.h:168-172) does not modify the buffer `b` at all. It only calls `ringBuffer->write(b, startSample, numSamples)` when the ring buffer is non-null and active. The `write()` method copies data from the buffer into the ring buffer without modifying the source. The audio output is identical to the input.

### buffer-write-condition

**Question:** Under what conditions does the ring buffer write occur?

**Answer:** The write occurs when `ringBuffer != nullptr && ringBuffer->isActive()` (Analyser.h:170). The `isActive()` check is on the ring buffer itself, which is controlled by whether a UI component is currently connected and consuming the data. When PreviewType is set to Nothing (value 1), `updateType()` (Analyser.h:89) does nothing - it does not register a property object. However, the ring buffer still exists and may still be active if a UI component is connected. The write is not explicitly gated by PreviewType.

### preview-type-ring-buffer

**Question:** How does PreviewType configure the DisplayBufferSource?

**Answer:** `updateType()` (Analyser.h:79-101) uses a `ScopedPropertyCreator` and registers a different property object for each type:
- Nothing: no property object registered (the switch case breaks without action)
- Goniometer: registers `scriptnode::analyse::Helpers::GonioMeter`
- Oscilloscope: registers `scriptnode::analyse::Helpers::Oscilloscope`
- SpectrumAnalyser: registers `scriptnode::analyse::Helpers::FFT`

Each property object configures how the ring buffer data is interpreted by the UI component. The ring buffer itself (2-channel, initially 8192 samples) is created once in the constructor and shared across all modes.

### buffer-size-behaviour

**Question:** Does changing BufferSize at runtime resize the ring buffer?

**Answer:** Yes. `setInternalAttribute(BufferSize, newValue)` calls `ringBuffer->setRingBufferSize(2, (int)newValue)` (Analyser.h:108), which resizes the ring buffer at runtime. The constructor sets an initial size of 8192 samples (Analyser.h:76). The range is 0-32768 with step 1. A size of 0 would create a zero-length buffer, which may cause the ring buffer to become inactive. Practical values depend on the visualisation type: larger buffers give better frequency resolution for FFT, but increase display latency.

### ui-components

**Question:** What FloatingTile panel types are available?

**Answer:** The `AudioAnalyserComponent::Panel` class (Analyser.h:202-231) is registered as FloatingTile panel `"AudioAnalyser"` (SET_PANEL_NAME at line 214). It has sub-indices:
- 0: Goniometer
- 1: Oscilloscope
- 2: Spectral Analyser

The panel connects to any AnalyserEffect processor via `getProcessorTypeId()` returning the AnalyserEffect class type. The backend editor uses `AnalyserEditor` (Analyser.cpp:62).

## Processing Chain Detail

1. **Ring buffer write** (per-block, negligible CPU): Copies the audio buffer into the ring buffer for UI display. Only executes when the buffer is active (UI component connected). Does not modify the audio.

No other processing stages exist.

## Interface Usage

**DisplayBufferSource** (via ProcessorWithStaticExternalData): Provides one display buffer (index 0) configured as a 2-channel ring buffer. The buffer is created in the constructor with `getDisplayBuffer(0)` and sized to 8192 samples. The global UI updater is connected for refresh timing. Each visualisation mode registers a different property object that determines how the data is interpreted by the UI.

**RoutingMatrix**: Inherited from MasterEffectProcessor. Standard stereo routing.

## CPU Assessment

- **Ring buffer write**: negligible (simple memory copy, only when UI is connected)
- **Overall baseline**: negligible
- **No scaling factors**: CPU is constant regardless of parameters

## UI Components

- FloatingTile: `AudioAnalyser` - displays goniometer, oscilloscope, or spectrum analyser visualisation
- Backend editor: `AnalyserEditor`

## Notes

The Analyser is the only MasterEffect that implements ProcessorWithStaticExternalData (providing the DisplayBufferSource interface). The ring buffer data is consumed by UI components and has no effect on the audio path. The sample rate is forwarded to the ring buffer in `prepareToPlay()` for correct frequency scaling in the FFT display.
