---
title: Dynamics
moduleId: Dynamics
type: Effect
subtype: MasterEffect
tags: [dynamics, mixing]
builderPath: b.Effects.Dynamics
screenshot: /images/v2/reference/audio-modules/dynamics.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: "Enabled stages", impact: low, note: "Each enabled stage adds one per-sample processing loop. All three enabled is still low-to-medium." }
seeAlso:
  - { id: SimpleGain, type: alternative, reason: "Simpler gain control without gate, compressor, or limiter stages" }
  - { id: "dynamics.comp", type: scriptnode, reason: "Scriptnode compressor -- scriptnode splits dynamics processing into separate nodes" }
  - { id: "dynamics.gate", type: scriptnode, reason: "Scriptnode noise gate" }
  - { id: "dynamics.limiter", type: scriptnode, reason: "Scriptnode peak limiter" }
commonMistakes:
  - title: "0 dB threshold disables all stages"
    wrong: "Enabling a stage but leaving its threshold at 0 dB and expecting an effect"
    right: "Lower the threshold below 0 dB to hear the gate, compressor, or limiter working"
    explanation: "All thresholds default to 0 dB. At 0 dB, the gate never closes, the compressor never engages, and the limiter never limits because the signal is always below the threshold."
  - title: "Ratio 1:1 means no compression"
    wrong: "Enabling the compressor with ratio 1:1 and expecting compression"
    right: "Increase the ratio above 1:1 to apply compression"
    explanation: "A ratio of 1:1 means no gain reduction. The default ratio is 1:1, so the compressor does nothing until the ratio is increased."
  - title: "Only limiter prevents toggle clicks"
    wrong: "Toggling the gate or compressor rapidly and hearing clicks"
    right: "Only the limiter has a crossfade on toggle. Gate and compressor switch instantly."
    explanation: "The limiter uses a one-block crossfade when enabled or disabled to prevent clicks. The gate and compressor do not have this mechanism, so toggling them during playback may produce audible artefacts."
  - title: "Do not automate limiter attack time"
    wrong: "Changing the limiter attack parameter during playback via automation or scripting"
    right: "Set the limiter attack time before playback and leave it static"
    explanation: "Changing the limiter attack mid-playback switches internal processing at a random point in the buffer, causing clicks and pops. [1]($FORUM_REF.2857$)"
  - title: "Hard knee only"
    wrong: "Looking for a soft knee parameter on the compressor"
    right: "The compressor uses a hard knee exclusively"
    explanation: "No knee control is exposed. For soft-knee compression, use a custom scriptnode compressor. [2]($FORUM_REF.3466$)"
forumReferences:
  - id: 1
    title: "Do not automate limiter attack time"
    summary: "Changing the limiter attack mid-playback switches internal processing at a random buffer point, causing clicks and pops."
    topic: 2857
  - id: 2
    title: "Hard knee only — no soft knee control"
    summary: "The chunkware SimpleComp algorithm uses a hard knee exclusively; there is no knee parameter exposed."
    topic: 3466
  - id: 3
    title: "Reduction parameters must be polled"
    summary: "CompressorReduction, GateReduction, and LimiterReduction do not push updates — poll them with getAttribute() inside a timer callback."
    topic: 1929
  - id: 4
    title: "FloatingTile slots for gain reduction meters"
    summary: "Set FloatingTile processor ID to the Dynamics module name; index 0 = gate, 1 = compressor, 2 = limiter."
    topic: 13755
  - id: 5
    title: "Sidechain filtering requires scriptnode"
    summary: "No built-in sidechain filter — route a filtered copy of the signal through a DSP network to drive frequency-dependent compression."
    topic: 2072
  - id: 6
    title: "Auto-makeup over-compensates at typical settings"
    summary: "The formula (1 - ratio) * threshold applies full theoretical gain at threshold, which is too hot in practice — scale by 0.7 or apply a logarithmic correction."
    topic: 1965
  - id: 7
    title: "Parallel compression requires routing matrix"
    summary: "The Dynamics module has no built-in dry/wet mix; use a Routing Matrix to split into dry and wet paths and mix them with a gain module."
    topic: 1745
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: medium
  description: "A scriptnode network with gate, compressor, and limiter nodes in series can replicate this module's three-stage dynamics chain"
llmRef: |
  Dynamics (MasterEffect)

  A three-stage dynamics processor combining a noise gate, downward compressor, and brick-wall limiter in a fixed serial chain. Each stage can be independently enabled or disabled. Disabled stages consume zero CPU. All processing is per-sample.

  Signal flow:
    audio in -> [Gate] -> [Compressor -> Compressor Makeup] -> [Limiter -> Limiter Makeup] -> audio out

  CPU: low (single stage) to low-medium (all stages). Monophonic.

  Parameters:
    Gate:
      GateEnabled (Off/On, default Off) - enables the noise gate
      GateThreshold (-100 - 0 dB, default 0 dB) - level below which the gate attenuates
      GateAttack (0 - 100 ms, default 1 ms) - gate open time
      GateRelease (0 - 300 ms, default 100 ms) - gate close time
      GateReduction (read-only) - current gain reduction meter
    Compressor:
      CompressorEnabled (Off/On, default Off) - enables the compressor
      CompressorThreshold (-100 - 0 dB, default 0 dB) - compression knee point
      CompressorRatio (1 - 32, default 1:1) - compression ratio
      CompressorAttack (0 - 100 ms, default 10 ms) - compressor response time
      CompressorRelease (0 - 300 ms, default 100 ms) - compressor recovery time
      CompressorReduction (read-only) - current gain reduction meter
      CompressorMakeup (Off/On, default Off) - auto makeup gain
    Limiter:
      LimiterEnabled (Off/On, default Off) - enables the brick-wall limiter
      LimiterThreshold (-100 - 0 dB, default 0 dB) - maximum output level
      LimiterAttack (0 - 100 ms, default 1 ms) - limiter response time
      LimiterRelease (0 - 300 ms, default 10 ms) - limiter recovery time
      LimiterReduction (read-only) - current gain reduction meter
      LimiterMakeup (Off/On, default Off) - auto makeup gain

  When to use:
    Channel strip dynamics on a stereo bus. Enable only the stages you need. Gate for noise removal, compressor for dynamic range control, limiter for peak protection.

  Common mistakes:
    All thresholds default to 0 dB (no effect). Compressor ratio defaults to 1:1 (no compression). Gate/compressor toggle instantly (may click).

  Custom equivalent:
    scriptnode HardcodedFX with gate, compressor, and limiter nodes in series.

  See also:
    alternative SimpleGain - simpler gain control without dynamics stages
    [scriptnode] dynamics.comp - scriptnode compressor -- scriptnode splits dynamics processing into separate nodes
    [scriptnode] dynamics.gate - scriptnode noise gate
    [scriptnode] dynamics.limiter - scriptnode peak limiter
---

::category-tags
---
tags:
  - { name: dynamics, desc: "Effects that shape the amplitude or add distortion and saturation" }
  - { name: mixing, desc: "Effects that control volume, stereo width, or stereo balance" }
---
::

![Dynamics screenshot](/images/v2/reference/audio-modules/dynamics.png)

The Dynamics module is a three-stage dynamics processor with a noise gate, downward compressor, and brick-wall limiter arranged in a fixed serial chain. Each stage can be independently enabled or disabled, and disabled stages are fully bypassed with zero CPU cost.

All three stages process per-sample for accurate envelope tracking. The compressor and limiter each offer an auto-makeup toggle that compensates for gain reduction based on the threshold (and ratio for the compressor). The three read-only Reduction parameters provide gain reduction metering for each stage.

## Signal Path

::signal-path
---
glossary:
  parameters:
    GateEnabled:
      desc: "Enables the noise gate stage"
      range: "Off / On"
      default: "Off"
    GateThreshold:
      desc: "Level below which the gate attenuates the signal"
      range: "-100 - 0 dB"
      default: "0 dB"
    GateAttack:
      desc: "Time for the gate to open"
      range: "0 - 100 ms"
      default: "1 ms"
    GateRelease:
      desc: "Time for the gate to close"
      range: "0 - 300 ms"
      default: "100 ms"
    CompressorEnabled:
      desc: "Enables the compressor stage"
      range: "Off / On"
      default: "Off"
    CompressorThreshold:
      desc: "Level above which compression begins"
      range: "-100 - 0 dB"
      default: "0 dB"
    CompressorRatio:
      desc: "Compression ratio (e.g. 4 means 4:1)"
      range: "1 - 32"
      default: "1 (no compression)"
    CompressorAttack:
      desc: "Compressor response time"
      range: "0 - 100 ms"
      default: "10 ms"
    CompressorRelease:
      desc: "Compressor recovery time"
      range: "0 - 300 ms"
      default: "100 ms"
    CompressorMakeup:
      desc: "Enables auto makeup gain after compression"
      range: "Off / On"
      default: "Off"
    LimiterEnabled:
      desc: "Enables the brick-wall limiter stage"
      range: "Off / On"
      default: "Off"
    LimiterThreshold:
      desc: "Maximum output level ceiling"
      range: "-100 - 0 dB"
      default: "0 dB"
    LimiterAttack:
      desc: "Limiter response time"
      range: "0 - 100 ms"
      default: "1 ms"
    LimiterRelease:
      desc: "Limiter recovery time"
      range: "0 - 300 ms"
      default: "10 ms"
    LimiterMakeup:
      desc: "Enables auto makeup gain after limiting"
      range: "Off / On"
      default: "Off"
  functions:
    gate:
      desc: "Per-sample noise gate that attenuates signal below the threshold"
    compress:
      desc: "Per-sample downward compressor that reduces gain above the threshold"
    limit:
      desc: "Per-sample brick-wall limiter that prevents signal from exceeding the threshold"
---

```
// Dynamics - monophonic, three-stage serial chain
// stereo in -> stereo out

// Stage 1: Gate
if GateEnabled:
    signal = gate(signal, GateThreshold, GateAttack, GateRelease)

// Stage 2: Compressor
if CompressorEnabled:
    signal = compress(signal, CompressorThreshold, CompressorRatio,
                      CompressorAttack, CompressorRelease)
    if CompressorMakeup:
        signal *= makeupGain(CompressorThreshold, CompressorRatio)

// Stage 3: Limiter (crossfades on enable/disable)
if LimiterEnabled:
    signal = limit(signal, LimiterThreshold, LimiterAttack, LimiterRelease)
    if LimiterMakeup:
        signal *= makeupGain(LimiterThreshold)

output = signal
```

::

## Parameters

::parameter-table
---
groups:
  - label: Gate
    params:
      - { name: GateEnabled, desc: "Enables the noise gate. When off, this stage is fully bypassed.", range: "Off / On", default: "Off" }
      - { name: GateThreshold, desc: "The level in dB below which the gate attenuates the signal. Lower values allow quieter signals through.", range: "-100 - 0 dB", default: "0 dB" }
      - { name: GateAttack, desc: "Time in milliseconds for the gate to open when the signal exceeds the threshold.", range: "0 - 100 ms", default: "1 ms" }
      - { name: GateRelease, desc: "Time in milliseconds for the gate to close when the signal falls below the threshold.", range: "0 - 300 ms", default: "100 ms" }
      - { name: GateReduction, desc: "Read-only. Displays the current gate gain reduction with peak-hold and exponential decay.", range: "(read-only)", default: "0" }
  - label: Compressor
    params:
      - { name: CompressorEnabled, desc: "Enables the downward compressor. When off, this stage is fully bypassed.", range: "Off / On", default: "Off" }
      - { name: CompressorThreshold, desc: "The level in dB above which compression begins.", range: "-100 - 0 dB", default: "0 dB" }
      - { name: CompressorRatio, desc: "Compression ratio. A value of 4 means 4:1 compression. 1 means no compression.", range: "1 - 32", default: "1" }
      - { name: CompressorAttack, desc: "Time in milliseconds for the compressor to respond to signals above the threshold.", range: "0 - 100 ms", default: "10 ms" }
      - { name: CompressorRelease, desc: "Time in milliseconds for the compressor to return to normal gain.", range: "0 - 300 ms", default: "100 ms" }
      - { name: CompressorReduction, desc: "Read-only. Displays the current compressor gain reduction.", range: "(read-only)", default: "0" }
      - { name: CompressorMakeup, desc: "Enables automatic makeup gain calculated from the threshold and ratio to compensate for compression.", range: "Off / On", default: "Off" }
  - label: Limiter
    params:
      - { name: LimiterEnabled, desc: "Enables the brick-wall limiter. Uses a one-block crossfade when toggled to prevent clicks.", range: "Off / On", default: "Off" }
      - { name: LimiterThreshold, desc: "The ceiling level in dB that the signal cannot exceed.", range: "-100 - 0 dB", default: "0 dB" }
      - { name: LimiterAttack, desc: "Time in milliseconds for the limiter to respond to peaks above the threshold.", range: "0 - 100 ms", default: "1 ms" }
      - { name: LimiterRelease, desc: "Time in milliseconds for the limiter to return to normal gain.", range: "0 - 300 ms", default: "10 ms" }
      - { name: LimiterReduction, desc: "Read-only. Displays the current limiter gain reduction.", range: "(read-only)", default: "0" }
      - { name: LimiterMakeup, desc: "Enables automatic makeup gain calculated from the threshold to compensate for limiting.", range: "Off / On", default: "Off" }
---
::

### Auto Makeup Gain

The compressor auto-makeup formula is `(1 - ratio) * threshold * -1`, which applies the full theoretical gain at the threshold point. In practice this tends to over-compensate at typical settings, producing output that is louder than the uncompressed signal. If auto-makeup is too hot, scale the result down manually (a factor of 0.7 is a common community workaround) or leave auto-makeup off and set output level explicitly. [6]($FORUM_REF.1965$)

### Parallel Compression

The Dynamics module has no built-in dry/wet mix parameter. To implement parallel (New York) compression, increase the container channel count to 4, use a Routing Matrix to copy the signal to channels 3–4, apply the Dynamics module only to channels 3–4, and mix the two paths back together using a gain module on a single knob. [7]($FORUM_REF.1745$)

### Gain Reduction Metering

The Reduction parameters (`GateReduction`, `CompressorReduction`, `LimiterReduction`) are output-only values for display purposes. They use peak-hold with exponential decay and must be polled via `getAttribute()` inside a timer callback - the module does not push updates. [3]($FORUM_REF.1929$) Gain reduction meters can be displayed using FloatingTile slots: index 0 for gate, 1 for compressor, 2 for limiter. [4]($FORUM_REF.13755$)

### Sidechain Filtering

The Dynamics module has no built-in sidechain filter for frequency-dependent compression (e.g., de-essing). For sidechain filtering, build a custom compressor in scriptnode that routes a filtered copy of the signal to drive the compression envelope. [5]($FORUM_REF.2072$)

**See also:** $MODULES.SimpleGain$ -- Simpler gain control without gate, compressor, or limiter stages, $SN.dynamics.comp$ -- scriptnode compressor -- scriptnode splits dynamics processing into separate nodes, $SN.dynamics.gate$ -- scriptnode noise gate, $SN.dynamics.limiter$ -- scriptnode peak limiter
