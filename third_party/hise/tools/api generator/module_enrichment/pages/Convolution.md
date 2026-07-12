---
title: Convolution Reverb
moduleId: Convolution
type: Effect
subtype: MasterEffect
tags: [reverb]
builderPath: b.Effects.Convolution
screenshot: /images/v2/reference/audio-modules/convolution.png
cpuProfile:
  baseline: very_high
  polyphonic: false
  scalingFactors:
    - { parameter: "IR length", impact: high, note: "Longer impulse responses increase tail convolution work proportionally" }
    - { parameter: UseBackgroundThread, impact: medium, note: "Moves tail processing off the audio thread but does not reduce total CPU" }
    - { parameter: FFTType, impact: medium, note: "Platform-optimised FFT implementations (IPP, Accelerate) are measurably faster than the fallback" }
seeAlso:
  - { id: SimpleReverb, type: alternative, reason: "Lightweight algorithmic reverb with much lower CPU cost but no impulse response loading" }
  - { id: Convolution, type: scriptnode, reason: "The fx.convolution scriptnode node shares the same DSP engine with additional routing flexibility" }
  - { id: "filters.convolution", type: scriptnode, reason: "Direct equivalent -- FFT partitioned convolution reverb" }
commonMistakes:
  - title: "Default convolution is 100% wet"
    wrong: "Adding a Convolution Reverb and expecting to hear dry signal"
    right: "Raise DryGain from -100 dB to 0 dB for a standard wet/dry blend"
    explanation: "The default DryGain is -100 dB (silent) and WetGain is 0 dB, so the output is 100% wet with no dry signal. This is unusual for a reverb - most users will want to raise DryGain."
  - title: "Damping and HiCut reshape IR offline"
    wrong: "Adjusting Damping or HiCut and expecting real-time filtering of the wet output"
    right: "These parameters reshape the impulse response itself, which triggers a full IR reload"
    explanation: "Damping and HiCut are applied to the impulse response offline during preparation, not as real-time filters. Changing them causes a brief reload with a crossfade."
  - title: "Long IRs need background threading"
    wrong: "Leaving UseBackgroundThread off with a long impulse response"
    right: "Enable UseBackgroundThread for long IRs to reduce audio thread load"
    explanation: "Without background threading, the entire convolution runs on the audio thread, which can cause dropouts with long impulse responses."
  - title: "IR list order differs between IDE and plugin"
    wrong: "Using loadAudioFilesIntoPool() index positions to recall IRs, assuming the order is stable"
    right: "Store and recall IRs by filename, not by array index"
    explanation: "The file list returned by loadAudioFilesIntoPool() can have a different order in the exported plugin compared to the HISE IDE. Saving a default user preset can also shuffle the order. Always match IRs by name."
forumReferences:
  - id: 1
    title: "IR resampling causes gain increase at non-native sample rates"
    summary: "When the host sample rate differs from the IR's native sample rate, resampling introduces a gain increase (~6dB at 96kHz vs 44.1kHz). No built-in compensation."
    topic: 2054
  - id: 2
    title: "Glitch when DAW playback starts at the exact beginning of the timeline"
    summary: "Starting DAW playback from position 0:0:0 causes an audible glitch in the Convolution Reverb output. Acknowledged by Christoph Hart, no workaround documented."
    topic: 7006
  - id: 3
    title: "IR preset recall requires a hidden slider or combobox as intermediary"
    summary: "HISE presets save control values, not script state. To make IR selection recallable, bind the selection to a hidden slider whose onControl callback calls setFile()."
    topic: 1672
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: simple
  description: "The fx.convolution scriptnode node wraps the same two-stage FFT convolution engine"
llmRef: |
  Convolution Reverb (MasterEffect)

  Zero-latency convolution reverb that convolves the input signal with a loaded impulse response. Uses a two-stage partitioned FFT architecture: a small head convolver for low latency and a larger tail convolver for the bulk of the IR, optionally on a background thread. Damping and HiCut shape the IR offline during preparation, not as real-time filters. A fixed 0.5x gain compensation is applied to the wet signal.

  Signal flow:
    IR preparation (offline): audio file -> resample to host rate -> apply Damping fadeout -> apply HiCut filtering -> initialise convolver engines
    Audio processing: audio in -> convolve (two-stage FFT) -> predelay -> wet gain -> 0.5x compensation -> sum with dry -> audio out

  CPU: very_high, monophonic (MasterEffect).
    Scaling: IR length (proportional), UseBackgroundThread (redistributes load), FFTType (platform FFT faster than fallback).

  Parameters:
    DryGain (-100 to 0 dB, default -100 dB) - dry output level. Default is silent.
    WetGain (-100 to 0 dB, default 0 dB) - wet output level. Subject to 0.5x gain compensation.
    Predelay (0-200 ms, default 0) - delay before reverb tail
    Damping (-100 to 0 dB, default 0 dB) - exponential fadeout applied to IR (offline, triggers reload)
    HiCut (20-20000 Hz, default 20000 Hz) - low-pass filtering applied to IR (offline, triggers reload)
    ProcessInput (On/Off, default On) - enables convolution with 60ms fade
    UseBackgroundThread (On/Off, default Off) - moves tail to background thread
    FFTType (0-4, default 0 BestAvailable) - FFT implementation: BestAvailable, IPP, AppleAccelerate, Ooura, FFTW3. BestAvailable auto-selects: IPP (Windows), Accelerate (macOS), Ooura (fallback).
    Latency (default 0) - this parameter has no effect
    ImpulseLength (default 1) - deprecated, has no effect

  When to use:
    High-quality reverb using real acoustic spaces or designed impulse responses. Best for static reverb characters. For lightweight algorithmic reverb, use Simple Reverb instead.

  Scripting usage:
    Access via Synth.getAudioSampleProcessor("id"), not Synth.getEffect().
    setFile() requires {PROJECT_FOLDER} prefix, case-sensitive in compiled plugins.
    IR selection must be bound to a control for preset recall.
    Call Engine.loadAudioFilesIntoPool() in onInit for IR embedding in exports.

  Common mistakes:
    Default DryGain is -100 dB (silent) - raise to 0 dB for wet/dry blend.
    Damping and HiCut reshape the IR offline, triggering a reload on change.
    Enable UseBackgroundThread for long IRs to avoid audio thread overload.
    IR list from loadAudioFilesIntoPool() may differ in order between IDE and plugin - match by name, not index.

  Custom equivalent:
    scriptnode HardcodedFX: fx.convolution node (same engine).

  See also:
    alternative SimpleReverb - lightweight algorithmic reverb, much lower CPU
    scriptnode fx.convolution - same DSP engine with scriptnode routing
    [scriptnode] filters.convolution - direct equivalent -- FFT partitioned convolution reverb
---

::category-tags
---
tags:
  - { name: reverb, desc: "Effects that simulate room acoustics and spatial reflections" }
---
::

![Convolution Reverb screenshot](/images/v2/reference/audio-modules/convolution.png)

A zero-latency convolution reverb that convolves the input signal with a loaded impulse response (IR) audio file. It uses a two-stage partitioned FFT architecture to keep latency low while handling long impulse responses efficiently. The tail portion of the convolution can optionally run on a background thread to reduce audio thread load.

Unlike most reverb effects, the default mix is 100% wet: DryGain starts at -100 dB (silent) while WetGain starts at 0 dB. Raise DryGain to hear the original signal alongside the reverb. Damping and HiCut do not filter the wet output in real time - they reshape the impulse response itself during an offline preparation step, which triggers a brief crossfade when changed. The reverb tail is cleared when voices are killed, preventing lingering reverb from previous notes. Starting DAW playback from the very beginning of the timeline (position 0:0:0) may produce a brief audible glitch in the reverb output [2]($FORUM_REF.7006$).

## Signal Path

::signal-path
---
glossary:
  parameters:
    DryGain:
      desc: "Dry output level in decibels, applied to the original input"
      range: "-100 - 0 dB"
      default: "-100 dB"
    WetGain:
      desc: "Wet output level in decibels, applied to the convolved signal"
      range: "-100 - 0 dB"
      default: "0 dB"
    Predelay:
      desc: "Delay before the reverb tail begins"
      range: "0 - 200 ms"
      default: "0 ms"
    Damping:
      desc: "Exponential fadeout applied to the IR (offline, triggers reload)"
      range: "-100 - 0 dB"
      default: "0 dB"
    HiCut:
      desc: "Low-pass cutoff applied to the IR (offline, triggers reload)"
      range: "20 - 20000 Hz"
      default: "20000 Hz"
    ProcessInput:
      desc: "Enables convolution processing with a 60ms fade transition"
      range: "Off / On"
      default: "On"
  functions:
    convolve:
      desc: "Two-stage partitioned FFT convolution: a small head block for low latency and a larger tail block for the remainder of the IR"
    applyDamping:
      desc: "Applies an exponential fadeout curve to the impulse response based on the Damping parameter"
    applyHiCut:
      desc: "Applies two cascaded one-pole low-pass filters with exponentially decreasing cutoff along the IR"
    predelay:
      desc: "Per-sample delay line that adds a time gap before the reverb tail"
---

```
// Convolution Reverb - monophonic, zero-latency
// stereo in -> stereo out

// IR Preparation (offline, on parameter change)
ir = loadAudioFile()
ir = resampleToHostRate(ir)
if Damping < 0 dB:
    ir = applyDamping(ir, Damping)
if HiCut < 20000 Hz:
    ir = applyHiCut(ir, HiCut)

// Audio processing (per block)
process(left, right) {
    wet = convolve(left, right, ir)

    if Predelay > 0:
        wet = predelay(wet, Predelay)

    // Apply gains (smoothed)
    dry = [left, right] * DryGain
    wet = wet * WetGain * 0.5    // fixed gain compensation

    output = dry + wet
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Mix Levels
    params:
      - { name: DryGain, desc: "Dry output level. The default is -100 dB, which is effectively silent. Raise this to hear the original signal alongside the reverb.", range: "-100 - 0 dB", default: "-100 dB" }
      - name: WetGain
        desc: "Wet output level for the convolved signal. A fixed 0.5x gain compensation is applied after this gain, so the effective maximum wet level is -6 dB."
        range: "-100 - 0 dB"
        default: "0 dB"
        hints:
          - type: warning
            text: "When the host sample rate is higher than the IR's native rate, internal resampling can introduce a gain increase (roughly +3 dB per octave of rate difference). There is no automatic compensation - adjust WetGain manually if needed. [1]($FORUM_REF.2054$)"
  - label: Impulse Response Shaping
    params:
      - { name: Damping, desc: "Applies an exponential fadeout to the impulse response. At 0 dB, the IR is unmodified. Lower values cause the IR tail to decay faster. This is applied offline during IR preparation and triggers a reload when changed.", range: "-100 - 0 dB", default: "0 dB" }
      - { name: HiCut, desc: "Low-pass cutoff frequency applied to the impulse response. At 20000 Hz, no filtering occurs. Lower values progressively remove high frequencies from the IR tail. Applied offline during IR preparation; triggers a reload when changed.", range: "20 - 20000 Hz", default: "20000 Hz" }
  - label: Timing
    params:
      - { name: Predelay, desc: "Adds a delay before the reverb tail. Applied in real time via a delay line on the convolved signal, before the wet gain stage.", range: "0 - 200 ms", default: "0 ms" }
  - label: Engine Configuration
    params:
      - { name: ProcessInput, desc: "Enables convolution processing. Toggling fades the wet signal in or out over 60ms to prevent clicks. When off, only the dry gain is applied.", range: "Off / On", default: "On" }
      - { name: UseBackgroundThread, desc: "Moves the tail convolution to a background thread, reducing audio thread load. The head convolution always runs on the audio thread. Automatically disabled during offline rendering.", range: "Off / On", default: "Off" }
      - { name: FFTType, desc: "Selects the FFT implementation for convolution. BestAvailable auto-selects the fastest option for the current platform. Changing this triggers an IR reload.", range: "BestAvailable, IPP, AppleAccelerate, Ooura, FFTW3", default: "BestAvailable", hints: [{ type: tip, text: "BestAvailable selects the fastest FFT for the current platform: IPP on Windows (if available), Accelerate on macOS, Ooura as fallback." }] }
  - label: Vestigial
    params:
      - { name: Latency, desc: "This parameter has no effect. It is stored for serialisation but does not influence the convolution engine.", range: "0.0 - 1.0", default: "0" }
      - { name: ImpulseLength, desc: "Deprecated. This parameter has no effect. Use the sample area of the impulse response display to adjust the active range instead.", range: "0.0 - 1.0", default: "1" }
---
::

### IR Selection and Presets

IR file selection via `setFile()` is not automatically saved in user presets. To make IR selection recallable, bind the selection index to a UI slider (hidden if needed) whose control callback calls `setFile()` with the corresponding filename. The slider value is then persisted by the preset system [3]($FORUM_REF.1672$).

For products with many impulse responses, use the Expansion system to package IRs separately rather than embedding all of them. Alternatively, uncheck 'Embed Audio Files' in the project settings and distribute the AudioResources.dat file to the user's AppData folder.

**See also:** $MODULES.SimpleReverb$ -- lightweight algorithmic reverb, much lower CPU, $SN.filters.convolution$ -- direct equivalent -- FFT partitioned convolution reverb
