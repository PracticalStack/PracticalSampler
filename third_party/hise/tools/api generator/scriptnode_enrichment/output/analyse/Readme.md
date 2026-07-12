---
title: Analyse Nodes
factory: analyse
---

The analyse factory contains nodes for visualising audio signals during development and on the end-user interface. All analyse nodes are pure passthrough -- they copy audio data into display buffers (or read processing context) without modifying the signal in any way. CPU cost on the audio thread is negligible for all nodes in this factory, as any heavy computation (such as FFT) runs asynchronously.

To display analysis visuals on your main user interface, register the node's display buffer as an external DisplayBufferSource, obtain a reference in script, and render the output.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.analyse.fft$]($SN.analyse.fft$) | Frequency spectrum analyser with configurable FFT size, windowing, and display properties |
| [$SN.analyse.goniometer$]($SN.analyse.goniometer$) | Stereo correlation display (Lissajous X-Y plot) for checking stereo width and phase |
| [$SN.analyse.oscilloscope$]($SN.analyse.oscilloscope$) | Time-domain waveform display with optional MIDI note-on synchronisation |
| [$SN.analyse.specs$]($SN.analyse.specs$) | Debug tool displaying processing context (sample rate, block size, channels, MIDI, polyphony) |
