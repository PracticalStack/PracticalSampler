---
title: Analyser
moduleId: Analyser
type: Effect
subtype: MasterEffect
tags: [utility]
builderPath: b.Effects.Analyser
screenshot: /images/v2/reference/audio-modules/analyser.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: AudioAnalyser, type: ui_component, reason: "FloatingTile that displays the Analyser's goniometer, oscilloscope, or spectrum visualisation" }
  - { id: "analyse.fft", type: scriptnode, reason: "Scriptnode FFT spectrum analyser node" }
  - { id: "analyse.oscilloscope", type: scriptnode, reason: "Scriptnode waveform oscilloscope display node" }
  - { id: "analyse.goniometer", type: scriptnode, reason: "Scriptnode stereo goniometer display node" }
commonMistakes:
  - title: "AudioAnalyser FloatingTile ignores standard colour properties"
    wrong: "Setting colour properties directly on the AudioAnalyser component"
    right: "Use a LookAndFeel override to style the AudioAnalyser display"
    explanation: "Standard colour properties have no effect on the AudioAnalyser component. The display must be styled through a custom LookAndFeel."
forumReferences:
  - id: 1
    title: "BufferSize setAttribute expects sample count, not a combo-box index"
    summary: "Pass the actual sample count (e.g. 4096, 8192) to setAttribute() for BufferSize, not a positional index."
    topic: 5320
  - id: 2
    title: "FFT display properties are not persisted; apply via setRingBufferProperties() each time"
    summary: "WindowType, DecibelRange, Decay, UseLogarithmicFreqAxis etc. must be set from script on every init using DisplayBuffer.setRingBufferProperties()."
    topic: 6666
  - id: 7
    title: "Use Engine.createFFT() for offline spectrum analysis instead"
    summary: "The Analyser module is real-time only; for offline or file-based spectrum analysis, Engine.createFFT() with setEnableSpectrum2D() is the correct approach."
    topic: 13231
llmRef: |
  Analyser (MasterEffect)

  A visualisation utility that captures audio data for display without modifying the signal. Provides goniometer (stereo phase), oscilloscope (waveform), and spectrum analyser (frequency content) views via the AudioAnalyser FloatingTile.

  Signal flow:
    audio in -> [copy to display buffer] -> audio out (passthrough, unchanged)

  CPU: negligible, monophonic.

  Parameters:
    PreviewType (Nothing, Goniometer, Oscilloscope, Spectral Analyser, default Nothing) - selects the visualisation mode
    BufferSize (0-32768 samples, default 8192) - ring buffer size, affects frequency resolution and display latency. setAttribute() expects the actual sample count, not a combo-box index.

  Tips:
    - FFT display properties (WindowType, DecibelRange, Decay, etc.) must be applied from script via DisplayBuffer.setRingBufferProperties() each time - they are not persisted by the editor.
    - getDisplayBuffer() argument is an arbitrary ID, not a channel index.
    - Use showControl(false) on the FloatingTile to toggle the display; there is no bypass API on the ring buffer.
    - For offline spectrum analysis, use Engine.createFFT() instead of this module.

  Common mistakes:
    - Setting colour properties on AudioAnalyser directly has no effect; use a LookAndFeel override.

  When to use:
    Monitoring stereo phase, waveform shape, or frequency content at any point in the effect chain. Insert wherever you need visual feedback.

  See also:
    ui_component AudioAnalyser - FloatingTile displaying the visualisation
    scriptnode analyse.fft - FFT spectrum analyser node
    scriptnode analyse.oscilloscope - waveform oscilloscope display node
    scriptnode analyse.goniometer - stereo goniometer display node
---

::category-tags
---
tags:
  - { name: utility, desc: "Modules for analysis, placeholders, or structural purposes without audio processing" }
---
::

![Analyser screenshot](/images/v2/reference/audio-modules/analyser.png)

The Analyser captures audio data for real-time visualisation without modifying the signal. It supports three display modes - goniometer for stereo phase correlation, oscilloscope for waveform monitoring, and spectrum analyser for frequency content - all rendered through the AudioAnalyser FloatingTile.

Audio passes through completely unchanged. The module only copies data into an internal ring buffer when a display component is connected and active, so it adds negligible CPU overhead.

## Parameters

::parameter-table
---
groups:
  - label: Visualisation
    params:
      - { name: PreviewType, desc: "Selects the visualisation mode. Each mode configures the display buffer for a different analysis type.", range: "Nothing, Goniometer, Oscilloscope, Spectral Analyser", default: "Nothing" }
      - { name: BufferSize, desc: "Size of the analysis ring buffer in samples. Larger values give better frequency resolution for the spectrum analyser but increase display latency.", range: "0 - 32768", default: "8192", hints: ["When setting via setAttribute(), pass the actual sample count (e.g. 8192, 16384), not a combo-box index [1]($FORUM_REF.5320$)."] }
---
::

### Configuring FFT Display Properties

Properties such as BufferLength, WindowType, DecibelRange, Decay, and UseLogarithmicFreqAxis are not persisted when set in the HISE editor. They must be applied each time from script using `DisplayBuffer.setRingBufferProperties()` with a JSON object [2]($FORUM_REF.6666$) [3]($FORUM_REF.11056$). The same property set works for the `analyse.fft` scriptnode node.

### Toggling the Display

There is no scripting API to bypass or disable the ring buffer from script. To hide the visualisation at runtime, call `showControl(false)` on the AudioAnalyser FloatingTile panel or toggle its visibility [4]($FORUM_REF.1951$) [5]($FORUM_REF.10897$). Without a connected display component, the module has zero processing overhead beyond the standard effect chain traversal.

### Display Buffer Index

The numeric argument passed to `getDisplayBuffer()` is an arbitrary ID used to distinguish multiple display buffers on the same source -- it is not a channel index [6]($FORUM_REF.12442$). Any numbering scheme (0-9 or otherwise) is valid.

### Offline Spectrum Analysis

The Analyser module is designed for real-time visualisation. For offline spectrum analysis of audio files or buffers (e.g. generating a 2D spectrogram), use `Engine.createFFT()` with `fft.setEnableSpectrum2D(true)` and `g.drawFFTSpectrum()` in a paint routine instead [7]($FORUM_REF.13231$).

**See also:** [AudioAnalyser](/v2/reference/floating-tiles/audioanalyser) -- FloatingTile that renders the goniometer, oscilloscope, or spectrum analyser visualisation from this module's display buffer, $SN.analyse.fft$ -- scriptnode FFT spectrum analyser, $SN.analyse.oscilloscope$ -- scriptnode waveform oscilloscope, $SN.analyse.goniometer$ -- scriptnode stereo goniometer
