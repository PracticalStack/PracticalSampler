# Scriptnode Bypass Infrastructure Reference

Distilled from C++ source for the node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/node_api/nodes/Bypass.h`
- `hi_dsp_library/node_api/nodes/Base.h`
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h`
- `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.cpp`
- `hi_scripting/scripting/scriptnode/api/NodeBase.h`
- `hi_scripting/scripting/scriptnode/api/NodeBase.cpp`
- `hi_scripting/scripting/scriptnode/api/StaticNodeWrappers.cpp`
- `hi_dsp_library/unit_test/wrapper_tests.cpp`

Infrastructure context:
- `scriptnode_enrichment/resources/infrastructure/core.md` (sfloat, PrepareSpecs)

---

## 1. Overview

Scriptnode has two bypass mechanisms:

1. **Hard bypass** (`bypass::simple`) -- immediately stops processing; can click
2. **Soft bypass** (`bypass::smoothed`) -- crossfades between dry and processed
   signal over a configurable duration; click-free

The `container.soft_bypass` node uses `bypass::smoothed` to provide soft
bypassing of its child chain in the scriptnode graph.

---

## 2. bypass::simple -- Hard Bypass Wrapper

Source: `Bypass.h`, lines 303-385

A template wrapper around any node T. Inherits from `SingleWrapper<T>`.

### Behavior when bypassed

- `process()` / `processFrame()`: skips inner node entirely (no-op)
- `reset()`: skips inner node
- `handleModulation()`: returns false (no modulation output)
- Audio passes through unmodified (dry signal)

### Behavior when active

- All calls forwarded directly to inner node

### setBypassed()

```cpp
void setBypassed(bool shouldBeBypassed)
{
    bypassed = shouldBeBypassed;
    if (bypassed)
        reset();  // reset inner node when entering bypass
}
```

Calls `reset()` on the inner node when entering bypass to clear internal state
(delay lines, filter coefficients, etc.).

### Parameter routing

Uses the special `bypass::ParameterId = 9000` constant. When `setParameter<P>()`
is called with `P == 9000`, it routes to `setBypassed(v > 0.5)`. Other parameter
indices are forwarded to the inner node.

---

## 3. bypass::smoothed -- Soft Bypass Wrapper

Source: `Bypass.h`, lines 46-234

The main bypass mechanism used by `container.soft_bypass`. Template parameters:
- `SmoothingTime` -- default crossfade duration in ms (-1 means configurable at runtime)
- `T` -- the wrapped node type

Inherits from `SingleWrapper<T>`.

### Template instantiation

```cpp
// Fixed 20ms crossfade (used in hardcoded/compiled networks)
bypass::smoothed<20, SomeNode>

// Runtime-configurable crossfade (used by container.soft_bypass)
bypass::smoothed<-1, SerialNode::DynamicSerialProcessor>
```

When `SmoothingTime == -1`, the smoothing time defaults to 20ms in the
constructor but can be changed at runtime via `setSmoothingTime()`.

### Internal state

```cpp
double sr = 0.0;        // sample rate (from prepare)
int smoothingTime;       // crossfade duration in ms
sfloat ramper;           // smoothed float for crossfade ramp
bool bypassed = false;   // current bypass state
```

The `sfloat ramper` (see core.md section 12) provides the linear ramp between
0.0 (bypassed) and 1.0 (active). The ramper's `isActive()` returns true during
the crossfade transition.

### prepare()

```cpp
void prepare(PrepareSpecs ps)
{
    sr = ps.sampleRate;
    ramper.prepare(ps.sampleRate, (double)smoothingTime);
    ramper.set(bypassed ? 0.0f : 1.0f);
    ramper.reset();       // jump to target immediately (no ramp on init)
    this->obj.prepare(ps);
}
```

After prepare, the ramper is at its target value with no active ramp. This
prevents an audible crossfade on initialization.

### setBypassed()

```cpp
void setBypassed(bool shouldBeBypassed)
{
    if (bypassed != shouldBeBypassed)
    {
        bypassed = shouldBeBypassed;
        ramper.set(bypassed ? 0.0f : 1.0f);

        if (!shouldBeBypassed)
            this->obj.reset();  // reset inner node when UN-bypassing
    }
}
```

Key behaviors:
- Only acts on state change (early return if same state)
- Sets ramper target: 0.0 = bypassed, 1.0 = active
- Calls `reset()` on inner node when transitioning FROM bypass TO active
  (not when entering bypass, unlike `bypass::simple`)
- Does NOT call `ramper.reset()` -- the ramp runs over the configured duration

### setSmoothingTime()

```cpp
void setSmoothingTime(int newTime)
{
    if constexpr (SmoothingTime == -1)
    {
        smoothingTime = jlimit(0, 1000, newTime);
        if (sr <= 0.0) return;
        ramper.prepare(sr, smoothingTime);
        ramper.set(bypassed ? 0.0f : 1.0f);
        ramper.reset();
    }
}
```

Only available when `SmoothingTime == -1` (runtime-configurable mode).
Clamps to 0-1000ms range. Re-prepares the ramper and jumps to current state.

---

## 4. Crossfade Algorithm (Block Processing)

Source: `Bypass.h`, lines 63-111

The crossfade runs during the transition period when `ramper.isActive()` is true.

### Algorithm steps

1. **Generate ramp data**: advance the ramper for each sample, clamped to [0, 1]
2. **Copy and pre-multiply**: copy dry signal to stack buffer, multiply by ramp
3. **Process wet**: run the inner node on the pre-multiplied copy
4. **Blend**: for each sample, mix dry and wet using the ramp value

```
ramp[i]    = ramper.advance()         // 0->1 (un-bypassing) or 1->0 (bypassing)
invRamp[i] = 1.0 - ramp[i]

wet_input  = dry * ramp               // pre-multiply input to processor
wet_output = process(wet_input)        // process the ramped input

output     = dry * invRamp + wet_output * ramp
```

### Double-ramp on wet signal (by design)

The wet signal is multiplied by the ramp value twice:
1. Before processing: the processor receives `dry * ramp` as input
2. After processing: the output is multiplied by `ramp` again

This means the effective wet contribution is `process(dry * ramp) * ramp`, not
`process(dry) * ramp`. This is intentional -- it prevents the processor from
seeing a sudden full-amplitude input during the transition, which would cause
artifacts with stateful effects (reverb tails, delay feedback, filter transients).

### Stack allocation

Both the ramp data and the wet signal buffer are allocated on the stack using
`alloca()`. The maximum channel count is `NUM_MAX_CHANNELS`.

### When not ramping

- If `bypassed == false` and ramper is inactive: forward directly to inner node
- If `bypassed == true` and ramper is inactive: pass-through (no processing)

---

## 5. Crossfade Algorithm (Frame Processing)

Source: `Bypass.h`, lines 130-153

Same logic as block processing but per-sample:

```cpp
if (shouldSmoothBypass())
{
    const auto rampValue = ramper.advance();
    const auto invRampValue = 1.0f - rampValue;

    FrameDataType wet = data;       // copy frame
    wet *= rampValue;               // pre-multiply
    this->obj.processFrame(wet);    // process
    data *= invRampValue;           // scale dry
    wet *= rampValue;               // post-multiply wet
    data += wet;                    // blend
}
```

Identical double-ramp behavior as the block path.

---

## 6. Modulation and Event Handling During Bypass

### handleModulation()

```cpp
bool handleModulation(double& value) noexcept
{
    if constexpr (prototypes::check::handleModulation<T>::value)
    {
        if (!bypassed)
            return this->obj.handleModulation(value);
    }
    return false;
}
```

When bypassed, modulation output is suppressed (returns false). This means
modulation targets connected to nodes inside a soft-bypassed container will
stop receiving updates immediately -- there is no smoothing on modulation output.

### handleHiseEvent()

Inherited from `SingleWrapper<T>`, which unconditionally forwards MIDI events
to the inner node:

```cpp
void handleHiseEvent(HiseEvent& e)
{
    obj.handleHiseEvent(e);
}
```

MIDI events are always forwarded regardless of bypass state. This ensures that
nodes inside the bypassed container maintain correct MIDI state (note tracking,
CC values, etc.) so they are ready when un-bypassed.

However, `SoftBypassNode` overrides this with its own forwarding that also goes
through the wrapper -- the behavior is the same (unconditional forwarding).

### reset()

```cpp
void reset()
{
    ramper.reset();       // jump ramper to target (cancel active ramp)
    this->obj.reset();    // reset inner node
}
```

Calling reset() cancels any in-progress crossfade and resets the inner node.
This happens during voice start in polyphonic contexts.

---

## 7. container.soft_bypass -- The Scriptnode Node

Source: `NodeContainerTypes.h`, lines 147-174; `NodeContainerTypes.cpp`, lines 1641-1697

`SoftBypassNode` is a `SerialNode` (serial container) that wraps its child
processing chain in a `bypass::smoothed<-1, SerialNode::DynamicSerialProcessor>`.

### Factory ID

`"container.soft_bypass"` (registered via `SCRIPTNODE_FACTORY`)

### Description

"Allows soft bypassing without clicks"

### Properties

- **SmoothingTime** (`NodePropertyT<int>`, default: 20ms) -- the crossfade
  duration in milliseconds. Exposed as a node property in the UI. When changed,
  calls `obj.setSmoothingTime(newTime)`.

### Bypass parameter connection

Only `SoftBypassNode` accepts bypass parameter connections. When another node
(e.g., `control.xfader`) tries to connect to a node's "Bypassed" parameter,
the connection system checks:

```cpp
if (auto validNode = dynamic_cast<SoftBypassNode*>(tn))
{
    p = new NodeBase::DynamicBypassParameter(tn, {});
}
else
{
    // Error: "Can't add a bypass here"
}
```

This means you cannot dynamically control bypass on regular containers --
only `container.soft_bypass` supports it.

### DynamicBypassParameter

The bridge between parameter connections and the bypass state:

```cpp
void DynamicBypassParameter::call(double v)
{
    setDisplayValue(v);
    bypassed = !enabledRange.contains(v) && enabledRange.getEnd() != v;
    ScopedUndoDeactivator sns(node);
    node->setBypassed(bypassed);
}
```

The `enabledRange` is hardcoded to `{0.5, 1.0}`. Values >= 0.5 are "active",
values < 0.5 trigger bypass. The undo manager is temporarily disabled during
bypass state changes to prevent bypass toggles from cluttering the undo history.

### SoftBypassNode::setBypassed()

```cpp
void SoftBypassNode::setBypassed(bool shouldBeBypassed)
{
    SerialNode::setBypassed(shouldBeBypassed);   // sets bypassState flag
    WrapperType::setParameter<bypass::ParameterId>(&this->obj, (double)shouldBeBypassed);
}
```

This calls through the static parameter dispatch, which reaches
`bypass::smoothed::setBypassed()` and triggers the crossfade ramp.

---

## 8. softbypass_switch Template

Source: `StaticNodeWrappers.cpp`, lines 756-817

A template node factory that creates a switching structure using soft bypass.
Available as `softbypass_switch2` through `softbypass_switch8`.

### Generated structure

```
container.chain (root)
  |-- control.xfader ("switcher", Mode: "Switch", NumParameters: N)
  |-- container.chain ("sb_container", horizontal layout)
       |-- container.soft_bypass ("sb1")
       |    |-- math.mul ("dummy")
       |-- container.soft_bypass ("sb2")
       |    |-- math.mul ("dummy")
       |-- ... (N total)
```

### How it works

1. A `Switch` parameter (range 0 to N-1, step 1) is exposed at the root
2. The parameter connects to `control.xfader` which outputs to switch targets
3. Each switch target connects to the `Bypassed` parameter of a `soft_bypass`
   container
4. The xfader activates exactly one container and bypasses the others
5. The crossfade in each soft_bypass container smooths the transition

This creates a click-free N-way switch where only one processing path is
active at a time, with smooth crossfading between paths.

---

## 9. bypass::no -- Disabled Bypass (Commented Out)

Source: `Bypass.h`, lines 236-300 (inside `#if 0`)

A bypass wrapper that does nothing -- all bypass methods are no-ops.
Currently disabled in the codebase. Was likely used as the "no bypass"
option in a bypass-mode template system that was simplified.

---

## 10. Voice State and Polyphonic Behavior

### Reset on un-bypass

When `bypass::smoothed::setBypassed(false)` is called (transitioning to active),
it calls `this->obj.reset()` on the inner node. This clears per-voice state
so the processor starts from a clean slate.

### Reset on voice start

When `reset()` is called externally (e.g., voice start), it:
1. Calls `ramper.reset()` -- jumps the crossfade ramp to its target value
   immediately, canceling any in-progress crossfade
2. Calls `this->obj.reset()` -- resets the inner processor

This means a new voice always starts with a clean bypass state (no residual
crossfade from a previous voice's bypass toggle).

### Polyphonic bypass

The bypass state (`bool bypassed`) is a single value shared across all voices.
There is no per-voice bypass. Toggling bypass affects all active voices
simultaneously. The crossfade ramper is also shared -- all voices hear the
same crossfade.

---

## 11. Key Constants and Ranges

| Constant | Value | Description |
|----------|-------|-------------|
| `bypass::ParameterId` | 9000 | Magic parameter index for bypass routing |
| Default smoothing time | 20ms | Both constructor default and property default |
| Smoothing time range | 0-1000ms | Enforced by `jlimit` in `setSmoothingTime` |
| Bypass threshold | 0.5 | Values >= 0.5 = active, < 0.5 = bypassed |
| `NUM_MAX_CHANNELS` | (global) | Max channels for stack-allocated wet buffer |

---

## 12. Crossfade Characteristics

- **Type:** Linear crossfade with double-ramp on wet signal
- **Duration:** Configurable (default 20ms, range 0-1000ms)
- **Ramp shape:** Linear (sfloat uses linear interpolation)
- **Energy profile:** Not equal-power. The double-ramp means the wet signal
  contribution peaks at `ramp^2` rather than `ramp`, so there is a slight dip
  in energy at the crossfade midpoint. At ramp=0.5: dry=0.5, wet=0.25,
  total=0.75 (vs. 1.0 for equal-power). This is acceptable for short crossfade
  times (20ms) where the dip is inaudible.
- **0ms smoothing:** When smoothing time is 0, the ramper completes instantly
  and `shouldSmoothBypass()` returns false after the first block, making it
  effectively a hard bypass.

---

## 13. Summary: Bypass State Machine

```
State: ACTIVE (bypassed=false, ramper inactive at 1.0)
  |
  | setBypassed(true)
  v
State: CROSSFADING TO BYPASS (bypassed=true, ramper ramping 1.0 -> 0.0)
  |  - Dry signal fades in, wet signal fades out
  |  - Inner node still processes (with diminishing input)
  |  - Modulation output suppressed immediately
  |
  | ramper reaches 0.0
  v
State: BYPASSED (bypassed=true, ramper inactive at 0.0)
  |  - Inner node not called at all
  |  - Audio passes through unmodified
  |
  | setBypassed(false) -- calls obj.reset()
  v
State: CROSSFADING TO ACTIVE (bypassed=false, ramper ramping 0.0 -> 1.0)
  |  - Inner node reset then starts processing
  |  - Wet signal fades in, dry signal fades out
  |
  | ramper reaches 1.0
  v
State: ACTIVE
```
