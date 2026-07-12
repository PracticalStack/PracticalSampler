# routing.public_mod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/RoutingNodes.h:351`
**Base class:** None (standalone, uses `SN_GET_SELF_AS_OBJECT`)
**Classification:** utility

## Signal Path

public_mod is not in the audio signal path. It is marked `OutsideSignalPath` and `IsPublicMod` (lines 373-374). All audio processing callbacks are empty (`SN_EMPTY_PROCESS`, `SN_EMPTY_PROCESS_FRAME`, etc.).

The node receives a value via its `Value` parameter and writes it to a `ModValue` pointer. This pointer is obtained during `prepare()` from `ps.voiceIndex->getTempoSyncer()->publicModValue` (line 368). The value is then accessible from outside the nested network/compiled node via the DllBoundaryTempoSyncer's publicModValue field.

When `setParameter<0>(v)` is called (line 380-383), it calls `ptr->setModValueIfChanged(v)` which only flags the ModValue as changed if the value actually differs from the current one.

## Gap Answers

### public-mod-mechanism: How does public_mod expose its Value as a modulation output on the parent network?

It writes to `DllBoundaryTempoSyncer::publicModValue`. In `prepare()` (line 367-369), if `ptr` is null, it obtains the pointer via `ps.voiceIndex->getTempoSyncer()->publicModValue`. The parent network reads from this same ModValue pointer. The `connect()` method (line 386-389) can also directly connect to a `public_mod_target` object's ModValue, bypassing the TempoSyncer mechanism.

### value-update-timing: When Value changes, is the public modulation output updated immediately?

The update is immediate. `setParameter<0>(v)` directly calls `ptr->setModValueIfChanged(v)` with no buffering or deferral. The ModValue is written synchronously. The `setModValueIfChanged()` method sets the value and flags it as changed only if the new value differs from the current one, providing basic change detection.

### multiple-public-mods: Can a nested network contain multiple public_mod nodes?

From this C++ source alone, there is only a single `publicModValue` pointer on the DllBoundaryTempoSyncer. The `prepare()` method always fetches the same `publicModValue` pointer. This suggests that a single nested network supports only one public_mod node through this mechanism. However, the `connect()` method accepts a `public_mod_target&` which could be used for additional connections in compiled code where targets are wired explicitly.

### normalised-range: Is the public modulation output always normalised 0..1?

The Value parameter is defined with range `{0.0, 1.0}` (line 393). The node does not clamp the value in `setParameter()` -- it passes whatever it receives to `setModValueIfChanged()`. However, the parameter range definition constrains the UI slider to 0..1. If driven by modulation from another node, values outside 0..1 could theoretically be passed through.

## Parameters

- **Value** (0.0 -- 1.0): The value to expose as a public modulation output. Written to the parent network's ModValue via the DllBoundaryTempoSyncer.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The comment in `prepare()` (line 366: "This is beyond ugly, but somebody has tucked a mod value to the temposyncer...") indicates this is an acknowledged architectural workaround. The `public_mod_target` struct (line 339-349) provides the receiving end with a standard `handleModulation(double& v)` method that reads from the shared ModValue.
