# control.pma - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:2613`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::pma>` (via alias at line 2735)
**Classification:** control_source

## Signal Path

Value parameter (0..1) -> multiply by Multiply -> add Add -> clamp to 0..1 -> modulation output.

The node uses `multilogic::pma` as its DataType, which inherits from `pma_base` (line 2544). The `pma` struct overrides `getValue()` at line 2621:

```
return jlimit(0.0, 1.0, value * mulValue + addValue);
```

Output is always clamped to [0, 1]. The `multi_parameter` template (line 2631) wraps this in `PolyData` and calls `sendPending()` after any parameter change.

## Gap Answers

### pma-formula-verification: In multilogic::pma::getValue(), is the formula exactly jlimit(0.0, 1.0, value * mulValue + addValue)? Does the clamping always apply?

Yes. The formula at line 2624 is exactly `jlimit(0.0, 1.0, value * mulValue + addValue)`. The clamping always applies unconditionally -- there is no branch or flag to disable it.

### pma-dirty-flag-semantics: When multiple parameters change in rapid succession, does sendPending() coalesce them into a single output, or does each parameter change trigger an output?

Each parameter change triggers `sendPending()` individually. In `setParameterStatic<P>()` (line 2652), the method iterates all voices to set the parameter, then calls `sendPending()`. Since `pma_base::setParameter<P>()` sets `dirty = true` on every call (line 2559), and `getValue()` clears `dirty` (line 2623), each parameter change results in one output call. There is no coalescing -- if Value, Multiply, and Add all change in sequence, three output calls occur.

### pma-polyphonic-voice-handling: In polyphonic mode, does setParameter iterate all voices while sendPending reads only the current voice?

Yes. `setParameterStatic<P>()` (line 2652-2656) uses `for(auto& s : typed->data)` which iterates all voices when called from the UI thread, or the current voice when called from the audio thread during rendering. `sendPending()` (line 2663-2687) checks `polyHandler->getVoiceIndex() == -1` and returns early if no voice is active (polyphonic path). When a voice IS active, it reads only `data.get()` which returns the current voice's data. So UI changes set all voices and send only if a voice is rendering; modulation changes during rendering set the current voice and send immediately.

## Parameters

- **Value** (P=0): Primary input signal, range 0..1. Stored as `value`.
- **Multiply** (P=1): Scale factor, range -1..1, default 1.0. Stored as `mulValue`.
- **Add** (P=2): DC offset, range -1..1, default 0.0. Stored as `addValue`.

## Polyphonic Behaviour

Uses `PolyData<multilogic::pma, NV>`. Each voice has independent value, mulValue, addValue, and dirty flag. In polyphonic mode, `sendPending()` only fires when `polyHandler->getVoiceIndex() != -1` (a voice is actively rendering).

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
