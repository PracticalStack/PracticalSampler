# container.soft_bypass -- C++ Exploration

**Source:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h:147` (interpreted), `hi_dsp_library/node_api/nodes/Bypass.h` (bypass::smoothed)
**Base class:** `SerialNode`
**Classification:** container

## Signal Path

Serial processing with smoothed bypass crossfading. When bypass is toggled,
a linear crossfade ramp transitions between dry (bypassed) and wet (active)
signals over a configurable duration, preventing clicks.

```
Active state:  Input --> Children process serially --> Output
Bypassed:      Input --> [passthrough] --> Output
Transition:    Input --> crossfade(dry, wet) --> Output
```

The crossfade uses a double-ramp on the wet signal:
```
wet_input  = dry * ramp
wet_output = process(wet_input)
output     = dry * (1-ramp) + wet_output * ramp
```

This means wet contribution is `process(dry * ramp) * ramp` (quadratic rolloff),
preventing stateful effects from seeing sudden full-amplitude input during transition.

## Gap Answers

### crossfade-algorithm: Confirm bypass::smoothed crossfade

**Confirmed.** The `bypass::smoothed` template (Bypass.h, per infrastructure/bypass.md)
implements a linear ramp with double-ramp on the wet signal.

Block processing (Bypass.h:63-111):
1. Generate ramp data via `ramper.advance()`, clamped to [0,1]
2. Copy dry signal to stack buffer, multiply by ramp
3. Process the pre-multiplied copy through the inner node
4. Blend: `output = dry * invRamp + wet_output * ramp`

The double-ramp causes a slight energy dip at the midpoint: at ramp=0.5,
dry=0.5, wet=0.25, total=0.75 (not equal-power). This is acceptable for
short crossfade times (default 20ms).

Both block and frame processing use the same algorithm. Stack allocation via
`alloca()` for the ramp data and wet buffer.

### bypass-parameter-connection: Unique dynamic bypass capability

**Confirmed.** Only `container.soft_bypass` accepts dynamic bypass parameter
connections. In `StaticNodeWrappers.cpp`, when a parameter connection targets
the "Bypassed" parameter, the system checks:
```cpp
if (auto validNode = dynamic_cast<SoftBypassNode*>(tn))
    p = new NodeBase::DynamicBypassParameter(tn, {});
else
    // Error: "Can't add a bypass here"
```

The `DynamicBypassParameter` (per infrastructure/bypass.md section 7) uses
`enabledRange = {0.5, 1.0}`: values >= 0.5 = active, < 0.5 = bypassed.
Undo manager is temporarily disabled during bypass state changes.

### modulation-during-bypass: Modulation and MIDI behavior

**Confirmed.** Per bypass.md section 6:

- **Modulation output:** Suppressed immediately when bypassed (returns false
  from `handleModulation()`). No smoothing on modulation -- it cuts instantly
  even though the audio crossfade takes time.
- **MIDI events:** Always forwarded regardless of bypass state. Children
  maintain correct MIDI state (note tracking, CC values) so they are ready
  when un-bypassed.

### smoothing-time-property-behavior: Property change during crossfade

**Confirmed.** `SoftBypassNode::updateSmoothingTime()` (NodeContainerTypes.cpp:1684-1691)
calls `obj.setSmoothingTime(newTime)`. The `bypass::smoothed::setSmoothingTime()`
(Bypass.h, per bypass.md section 3) re-prepares the ramper and jumps to current
state via `ramper.reset()`. This cancels any in-progress crossfade and snaps
to the current bypass state.

### polyphonic-bypass-sharing: Shared bypass state

**Confirmed.** The bypass state is a single `bool bypassed` shared across all
voices (per bypass.md section 10). The crossfade ramper is also shared. Toggling
bypass affects all voices simultaneously with the same crossfade. There is no
per-voice bypass.

## Parameters

None. Soft_bypass has no audio parameters. Bypass is controlled via the
`setBypassed()` method and the DynamicBypassParameter connection system.

## Properties

- **SmoothingTime** (NodePropertyT<int>, default 20): Crossfade duration in
  milliseconds. Range 0-1000ms. 0ms effectively becomes a hard bypass.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

During active state: zero overhead beyond serial child processing.
During crossfade: additional buffer copy, multiply, and blend operations
(one block's worth). During bypassed state: zero CPU (children not processed).

## Notes

- The `template.softbypass_switchN` nodes use soft_bypass internally for
  click-free N-way switching (per bypass.md section 8).
- `SoftBypassNode::setBypassed()` (NodeContainerTypes.cpp:1693-1697) calls
  both `SerialNode::setBypassed()` (sets bypassState flag) and
  `WrapperType::setParameter<bypass::ParameterId>()` (triggers the crossfade ramp).
- Constructor registers `smoothingTime` property and sets up the
  `updateSmoothingTime` callback (lines 1641-1649).
