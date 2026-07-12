---
title: Convolution
description: "A partitioned FFT convolution reverb that convolves the input signal with an impulse response loaded from an AudioFile slot."
factoryPath: filters.convolution
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/convolution.png
polyphonic: false
tags: [filters, convolution, reverb, impulse-response, ir, fft]
cpuProfile:
  baseline: high
  polyphonic: false
  scalingFactors:
    - { parameter: "IR length", impact: "linear", note: "Longer impulse responses increase FFT partition count and processing time" }
seeAlso:
  - { id: "fx.reverb", type: alternative, reason: "Algorithmic reverb with lower CPU cost" }
  - { id: "Convolution", type: module, reason: "Direct equivalent -- FFT partitioned convolution reverb" }
commonMistakes:
  - title: "Always monophonic, not per-voice"
    wrong: "Placing filters.convolution inside a polyphonic voice chain expecting per-voice reverb"
    right: "Place convolution on a monophonic bus (after voice summing). It is always monophonic."
    explanation: "Convolution is inherently expensive and stateful. The node processes only the summed signal regardless of where it is placed. For per-voice spatial effects, use simpler filters or delays instead."
  - title: "Damping and HiCut trigger IR reload"
    wrong: "Modulating Damping or HiCut in real time expecting smooth changes"
    right: "Damping and HiCut are applied during impulse response loading, not in real time. Changes trigger a reload."
    explanation: "These parameters pre-process the impulse response. Changing them causes the IR to be reloaded with crossfading, which is not suited for continuous modulation."
  - title: "Call Engine.loadAudioFilesIntoPool() before exporting"
    wrong: "Exporting a plugin without calling Engine.loadAudioFilesIntoPool() first"
    right: "Call Engine.loadAudioFilesIntoPool() in onInit to ensure all IRs are embedded in the compiled plugin."
    explanation: "The audio file pool does not load all files by default. At export time only files currently in use are embedded. Without this call, IR files will be missing in the compiled plugin."
  - title: "IR file paths are case-sensitive in compiled plugins"
    wrong: "Using a filename case that differs from the actual file on disk (e.g. '.wav' when the file is '.WAV')"
    right: "Ensure the exact case of the filename matches between your setFile() call and the file on disk."
    explanation: "In the HISE IDE on Windows, file paths are case-insensitive. In a compiled plugin, the path is used as a case-sensitive string key on all platforms. A mismatch will silently fail to load the IR."
llmRef: |
  filters.convolution

  Partitioned FFT convolution reverb with impulse response loading from an AudioFile slot. Mixes wet signal at approximately 50% with the dry input. Monophonic only.

  Signal flow:
    audio in -> FFT convolution with IR -> predelay -> wet/dry mix -> audio out

  CPU: high, monophonic. Scales linearly with IR length. Multithread offloads tail to background thread.

  Parameters:
    Gate: Off / On (default On). Enables/disables processing with click-free ramp.
    Predelay: 0 - 1000 ms (default 0). Delay applied to wet signal.
    Damping: -100 - 0 dB (default 0). Exponential decay applied to IR during loading.
    HiCut: 20 - 20000 Hz (default 20000). Frequency-dependent decay applied to IR during loading.
    Multithread: Off / On (default Off). Offloads tail convolution to background thread.

  When to use:
    Reverb and IR-based effects in scriptnode. Place on a bus, not per-voice. Use Gate to save CPU when convolution is not needed.

  Common mistakes:
    Not polyphonic - always processes summed output.
    Damping and HiCut are not real-time - they trigger IR reload.
    Must call Engine.loadAudioFilesIntoPool() in onInit before exporting or IRs will be missing.
    IR file paths are case-sensitive in compiled plugins even if they are not in the HISE IDE.
    Engine.getSampleRate() returns undefined in onInit inside a compiled plugin -- read it in prepareToPlay instead.

  See also:
    [alternative] fx.reverb - algorithmic reverb with lower CPU
    [module] Convolution - direct equivalent -- FFT partitioned convolution reverb
forumReferences:
  - { tid: 819, summary: "Engine.loadAudioFilesIntoPool() required for IR embedding" }
  - { tid: 1139, summary: "Case-sensitive IR file paths in compiled plugins" }
  - { tid: 2054, summary: "Convolution gain increase at high sample rates" }
---

A partitioned FFT convolution reverb that convolves the input signal with an impulse response loaded from an AudioFile slot. The wet signal is mixed at approximately 50% with the dry input. This is the only filter node that is not polyphonic - it always processes the summed signal on a monophonic bus.

![Convolution screenshot](/images/v2/reference/scriptnodes/filters/convolution.png)

Damping and HiCut are pre-processing parameters applied to the impulse response during loading, not in real time. Changing them triggers a crossfaded IR reload. Use the Gate parameter to disable convolution entirely when it is not needed, saving CPU.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gate:
      desc: "Enables or disables convolution processing"
      range: "Off / On"
      default: "On"
    Predelay:
      desc: "Delay applied to the wet signal"
      range: "0 - 1000 ms"
      default: "0"
    Damping:
      desc: "Exponential decay applied to the IR during loading"
      range: "-100 - 0 dB"
      default: "0"
    HiCut:
      desc: "Frequency-dependent decay applied to the IR during loading"
      range: "20 - 20000 Hz"
      default: "20000"
    Multithread:
      desc: "Offloads tail FFT to a background thread"
      range: "Off / On"
      default: "Off"
  functions:
    fftConvolve:
      desc: "Partitioned FFT convolution with the loaded impulse response"
---

```
// filters.convolution - FFT convolution reverb
// audio in -> audio out (wet/dry mix)

process(input) {
    if Gate == Off:
        output = input    // bypass, saves CPU
        return

    wet = fftConvolve(input, impulseResponse)

    if Predelay > 0:
        wet = delay(wet, Predelay)

    output = input * 0.5 + wet * 0.5    // fixed wet/dry mix
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Gate, desc: "Enables or disables convolution processing. When off, only the dry signal passes through, saving CPU. Transitions use a click-free ramp.", range: "Off / On", default: "On" }
      - { name: Predelay, desc: "Delay applied to the wet (convolved) signal before mixing.", range: "0 - 1000 ms", default: "0" }
  - label: IR Processing
    params:
      - { name: Damping, desc: "Exponential decay applied to the impulse response during loading. Lower values shorten the reverb tail. 0 dB means no damping. Changes trigger an IR reload.", range: "-100 - 0 dB", default: "0" }
      - { name: HiCut, desc: "Frequency-dependent decay applied to the impulse response during loading. Simulates high-frequency absorption - lower values make the tail darker. 20000 Hz means no filtering. Changes trigger an IR reload.", range: "20 - 20000 Hz", default: "20000" }
  - label: Performance
    params:
      - { name: Multithread, desc: "When on, the tail portion of the convolution runs on a background thread, reducing audio thread load. Automatically disabled during offline rendering.", range: "Off / On", default: "Off" }
---
::

### Setup

The node does not support frame-based processing. It must be placed in a block-processing context (not inside a [container.frame2_block]($SN.container.frame2_block$)).

The AudioFile slot accepts audio files directly. SampleMap and SFZ references are not supported. To access the convolution node from HiseScript, use `Synth.getAudioSampleProcessor()` -- not `Synth.getEffect()`. The AudioSampleProcessor reference exposes `setFile()` for loading IRs. Alternatively, set the audio file slot to External in the node UI and use `getAudioFile(0)` on the ScriptFX reference; this pattern also works after compiling the network to C++.

### Embedding IRs for Export

Call `Engine.loadAudioFilesIntoPool()` in onInit before exporting. Without this, only files currently in use are embedded in the compiled plugin and other IRs in the AudioFiles folder will be missing. This is the most common cause of missing IR resources in compiled plugins.

### File Path Case Sensitivity

IR file paths are case-sensitive in compiled plugins on all platforms, even though they may be case-insensitive in the HISE IDE on Windows. Ensure the filename string passed to `setFile()` matches the exact case of the file on disk. A mismatch will silently fail to load the IR.

### Sample Rate Considerations

`Engine.getSampleRate()` returns undefined in onInit inside an exported plugin because the sample rate is not yet known at that point. To read the sample rate (e.g. for loading rate-specific IRs), use the `prepareToPlay` callback of a Script FX module instead.

### IR Preset Recall

The convolution node does not automatically save which IR is loaded as part of a preset. To persist the selected IR across preset changes, store the IR selection as an integer index in a hidden knob or slider. The preset system saves the knob value, and the control callback reloads the correct IR on recall using the index into an array of IR file references.

### Wet/Dry Control

The wet/dry mix is fixed at approximately 50% and cannot be adjusted. To control the wet/dry balance, place the convolution node in a [container.split]($SN.container.split$) with a [control.xfader]($SN.control.xfader$) controlling the mix between the dry path and the convolution path.

### AudioFile Range

The Range parameter on the AudioFile slot can be used to select a portion of a longer audio file as the impulse response, allowing multiple IRs to be packed into a single file.

**See also:** $SN.fx.reverb$ -- algorithmic reverb with lower CPU cost, $MODULES.Convolution$ -- direct equivalent -- FFT partitioned convolution reverb
