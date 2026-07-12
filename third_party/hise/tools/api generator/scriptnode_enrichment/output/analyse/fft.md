---
title: FFT
description: "A spectrum analyser that displays the frequency content of the audio signal as an FFT graph."
factoryPath: analyse.fft
factory: analyse
polyphonic: false
tags: [analyse, fft, spectrum, frequency]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "analyse.oscilloscope", type: alternative, reason: "Time-domain waveform display instead of frequency domain" }
  - { id: "analyse.goniometer", type: alternative, reason: "Stereo correlation display instead of frequency content" }
  - { id: "Analyser", type: module, reason: "FFT spectrum analyser" }
commonMistakes:
  - title: "FFT property changes do not persist"
    wrong: "Editing FFT properties via the popup editor and expecting them to survive a reload"
    right: "Set properties via script using DisplayBuffer.setRingBufferProperties() to persist your configuration."
    explanation: "Manual edits in the popup editor revert to defaults when the patch is reloaded. Use the scripting API to set properties programmatically for permanent changes."
  - title: "Only the first channel is analysed"
    wrong: "Expecting the FFT to show a combined stereo spectrum"
    right: "The FFT extracts the first channel only. Place it after a mono mixdown if you need a combined reading."
    explanation: "The node is hardcoded to mono analysis. Stereo input passes through unmodified, but only the left channel feeds the FFT display."
llmRef: |
  analyse.fft

  Spectrum analyser that writes audio to a ring buffer for asynchronous FFT computation. Audio passes through unmodified (mono extraction for analysis only). Nine configurable display properties control FFT size, windowing, decibel range, and axis scaling.

  Signal flow:
    audio in (first channel) -> ring buffer write -> audio out (unchanged)
    (FFT computed asynchronously off the audio thread)

  CPU: negligible, monophonic

  Parameters:
    None (all configuration via display buffer properties).

  Display buffer properties:
    BufferLength: 1024 - 32768 samples (default 8192). FFT window size.
    WindowType: Windowing function (default Blackman Harris).
    Overlap: 0.0 - 0.875 (default 0.0). Analysis overlap.
    DecibelRange: Two-element array (default [-50, 0]). Display range in dB.
    UsePeakDecay: Off / On (default Off). Holds peak values.
    UseDecibelScale: Off / On (default On). Vertical axis in decibels.
    YGamma: 0.1 - 32.0 (default 1.0). Vertical axis curve.
    Decay: 0.0 - 0.99999 (default 0.7). Visual smoothing.
    UseLogarithmicFreqAxis: Off / On (default On). Logarithmic frequency axis.

  When to use:
    Visualising the frequency content of a signal during development, or displaying a spectrum on the end-user interface via DisplayBufferSource. Appears in 2 surveyed networks.

  Common mistakes:
    FFT property edits in the popup do not persist -- use script API instead.
    Only the first channel is analysed -- mono extraction is hardcoded.

  See also:
    [alternative] analyse.oscilloscope - time-domain waveform display
    [alternative] analyse.goniometer - stereo correlation display
    [module] Analyser - module-tree FFT spectrum analyser
---

Analyses the frequency content of the audio signal and displays it as an FFT spectrum graph. Audio passes through the node unmodified -- the node copies the first channel into a ring buffer where the FFT is computed asynchronously, outside the audio thread. This means the node adds virtually no CPU load to the audio processing chain regardless of the FFT size.

To show the spectrum on your main user interface, register the display buffer as an external [DisplayBufferSource]($API.DisplayBufferSource$), obtain a reference in script, and render the output. All FFT display properties (window type, buffer length, decibel range, axis scaling) are configured through the display buffer's property object rather than through node parameters.

## Signal Path

::signal-path
---
glossary:
  functions:
    writeToRingBuffer:
      desc: "Copies the first channel of audio into the display ring buffer for asynchronous FFT analysis"
---

```
// analyse.fft - frequency spectrum analyser
// audio in -> audio out (unchanged)

analyse(input) {
    writeToRingBuffer(input.channel[0])
    output = input    // all channels pass through
}
```

::

## Display Buffer Properties

The FFT display is configured through the ring buffer's property object, not through node parameters. Set properties in script using [DisplayBuffer.setRingBufferProperties]($API.DisplayBuffer.setRingBufferProperties$) with a JSON object:

```javascript
{
  "BufferLength": 8192,
  "WindowType": "Blackman Harris",
  "DecibelRange": [-50.0, 0.0],
  "UsePeakDecay": false,
  "UseDecibelScale": true,
  "YGamma": 1.0,
  "Decay": 0.7,
  "UseLogarithmicFreqAxis": true
}
```

| Property | Description | Range | Default |
|----------|-------------|-------|---------|
| BufferLength | Number of samples for the FFT window. Larger values give finer frequency resolution. | 1024 - 32768 | 8192 |
| WindowType | Windowing function applied before the FFT. | Blackman Harris, Hann, etc. | Blackman Harris |
| Overlap | Analysis overlap factor. Higher values give smoother updates at the cost of more computation. | 0.0 - 0.875 | 0.0 |
| DecibelRange | Minimum and maximum decibel values for the display. | Two-element array | [-50, 0] |
| UsePeakDecay | When enabled, peak values are held and decay gradually. | Off / On | Off |
| UseDecibelScale | Switches the vertical axis between linear and decibel scaling. | Off / On | On |
| YGamma | Gamma curve for the vertical axis. Values below 1.0 expand the lower range. | 0.1 - 32.0 | 1.0 |
| Decay | Visual smoothing factor. Higher values produce a slower, smoother display. | 0.0 - 0.99999 | 0.7 |
| UseLogarithmicFreqAxis | Switches the horizontal axis between linear and logarithmic frequency scaling. | Off / On | On |

### Buffer Configuration

Changing BufferLength or Overlap resizes the internal ring buffer. These properties interact: overlap effectively multiplies the buffer size to allow overlapping analysis windows.

> [!Tip:Set FFT properties via script for persistence] Editing properties through the popup editor in the node UI is convenient for experimentation, but these changes revert when the patch is reloaded. Use the scripting API to set properties programmatically.

**See also:** $SN.analyse.oscilloscope$ -- time-domain waveform display, $SN.analyse.goniometer$ -- stereo correlation display, $MODULES.Analyser$ -- module-tree spectrum analyser with FFT, oscilloscope, and goniometer modes
