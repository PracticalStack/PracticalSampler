---
title: Oscilloscope
description: "A waveform display with optional MIDI note-on synchronisation that locks the view to a single cycle."
factoryPath: analyse.oscilloscope
factory: analyse
polyphonic: false
tags: [analyse, oscilloscope, waveform, midi-sync]
screenshot: /images/v2/reference/scriptnodes/analyse/oscilloscope.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "analyse.fft", type: alternative, reason: "Frequency-domain spectrum display instead of time-domain waveform" }
  - { id: "analyse.goniometer", type: alternative, reason: "Stereo correlation display instead of time-domain waveform" }
  - { id: "Analyser", type: module, reason: "Waveform oscilloscope display" }
commonMistakes:
  - title: "MIDI sync requires a MIDI processing context"
    wrong: "Expecting automatic cycle sync when the oscilloscope is outside a MIDI-aware container"
    right: "Place the oscilloscope inside a container.midichain or polyphonic context so it receives note-on events."
    explanation: "The oscilloscope only resizes its buffer to show one cycle when it receives MIDI note-on events. Without a MIDI processing context, it displays raw audio at the default buffer length."
llmRef: |
  analyse.oscilloscope

  Time-domain waveform display. Writes audio to a ring buffer for visualisation. When placed in a MIDI processing context, note-on events dynamically resize the buffer to show exactly one cycle of the played frequency. Audio passes through unmodified.

  Signal flow:
    audio in -> ring buffer write -> audio out (unchanged)
    (optional) MIDI note-on -> calculate cycle length -> resize buffer

  CPU: negligible, monophonic

  Parameters:
    None.

  Display buffer properties:
    BufferLength: 128 - 65536 samples (default 8192).
    NumChannels: 1 - 2 (default 1).

  When to use:
    Inspecting waveform shape during sound design. MIDI sync is particularly useful for viewing oscillator output locked to pitch. Appears in 1 surveyed network.

  Common mistakes:
    MIDI sync requires a MIDI processing context -- place inside midichain or polyphonic container.

  See also:
    [alternative] analyse.fft - frequency spectrum display
    [alternative] analyse.goniometer - stereo correlation display
    [module] Analyser - module-tree analyser with oscilloscope mode
---

![Oscilloscope screenshot](/images/scriptnode/oscilloscope.png)

Displays the waveform of the audio signal in real time. Audio passes through the node completely unmodified while a copy is written to a ring buffer for visualisation.

The key feature is optional MIDI synchronisation: when placed inside a MIDI processing context (such as [container.midichain]($SN.container.midichain$) or a polyphonic network), the oscilloscope responds to note-on events by resizing the display buffer to show exactly one cycle of the played frequency. This locks the waveform view to the current pitch, making it straightforward to inspect oscillator waveshapes and harmonic content as notes are played. Without MIDI input, the oscilloscope displays raw audio at the default buffer length.

## Signal Path

::signal-path
---
glossary:
  functions:
    writeToRingBuffer:
      desc: "Copies audio into the display ring buffer for waveform visualisation"
    resizeToOneCycle:
      desc: "Calculates sample count for one cycle at the note frequency and resizes the display buffer"
---

```
// analyse.oscilloscope - waveform display with MIDI sync
// audio in -> audio out (unchanged)

analyse(input) {
    writeToRingBuffer(input)
    output = input
}

onNoteOn(event) {
    cycleLength = sampleRate / event.frequency
    resizeToOneCycle(cycleLength)
}
```

::

### Display Buffer Configuration

The display buffer defaults to 8192 samples and accepts values between 128 and 65536. The number of displayed channels is configurable between 1 and 2 via the display buffer properties.

### MIDI Synchronisation

When MIDI sync is active, each note-on recalculates the buffer size. The calculation is straightforward: `numSamples = sampleRate / frequency`. This means lower notes use longer buffers and higher notes use shorter ones.

### UI Integration

To display the oscilloscope on your main user interface, register the display buffer as an external [DisplayBufferSource]($API.DisplayBufferSource$) and render the output from script.

**See also:** $SN.analyse.fft$ -- frequency spectrum display, $SN.analyse.goniometer$ -- stereo correlation display, $MODULES.Analyser$ -- module-tree analyser with oscilloscope mode
