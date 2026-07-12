# template.softbypass_switch2 - Composite Exploration

**Root container:** `container.chain`
**Source:** `StaticNodeWrappers.cpp:756` (template factory `softbypass_switch<2>`)
**Classification:** container (composite template)

## Composite Structure

```
container.chain (root, IsVertical=true)
  +-- control.xfader "switcher" (Mode=Switch, NumParameters=2)
  |     SwitchTarget[0] -> sb1.Bypassed
  |     SwitchTarget[1] -> sb2.Bypassed
  +-- container.chain "sb_container" (IsVertical=false)
        +-- container.soft_bypass "sb1" (SmoothingTime=20)
        |     +-- math.mul "dummy" (Value=1.0)   [USER INSERTION POINT]
        +-- container.soft_bypass "sb2" (SmoothingTime=20)
              +-- math.mul "dummy" (Value=1.0)   [USER INSERTION POINT]
```

## Signal Path

Audio enters the root container.chain and flows serially through two children:

1. **control.xfader "switcher"**: This is a control node (outside the signal path). It does not process audio. It receives the Switch parameter value and outputs binary bypass states to the soft_bypass containers via SwitchTargets.

2. **container.chain "sb_container"**: Contains the two soft_bypass containers in serial. Since these are in a chain (serial), audio passes through sb1 then sb2. However, at any time only ONE is active (unbypassed) while the other is soft-bypassed (passing audio through unmodified). The result is that only the active path's processing is applied.

The effective signal flow is: Audio -> (active soft_bypass child processes it) -> Output. The bypassed soft_bypass child passes audio through transparently (dry signal), so chaining them serially is equivalent to selecting one processing path.

## Gap Answers

### internal-topology: Confirm xfader Switch mode activates exactly one soft_bypass child

Confirmed. The control.xfader in Switch mode sends binary 0/1 values to its SwitchTargets. For a given integer Switch value K (0-based), the xfader sets SwitchTarget[K] to "active" (value >= 0.5) and all other targets to "bypassed" (value < 0.5). Each SwitchTarget connects to one soft_bypass container's Bypassed parameter via DynamicBypassParameter. The enabledRange for bypass is {0.5, 1.0} -- values >= 0.5 are active, < 0.5 trigger bypass. So exactly one soft_bypass container is active at a time.

### crossfade-behaviour: How does the crossfade work during transitions?

When the Switch parameter changes from index A to index B:
1. The xfader sends "bypass" (value < 0.5) to sb[A+1] and "active" (value >= 0.5) to sb[B+1]
2. sb[A+1] begins a crossfade from active to bypassed: its sfloat ramper ramps from 1.0 to 0.0 over the SmoothingTime (default 20ms). During the ramp, the wet signal fades out and the dry signal fades in.
3. sb[B+1] begins a crossfade from bypassed to active: its sfloat ramper ramps from 0.0 to 1.0 over the SmoothingTime. During the ramp, the wet signal fades in and the dry signal fades out. The inner node is reset() first.

Both crossfades happen simultaneously. There is no gap -- both containers process during their respective ramp periods. The crossfade uses a linear ramp with double-ramp on the wet signal (see bypass.md section 4). At the midpoint (ramp=0.5), there is a slight energy dip (total ~0.75 vs 1.0 for equal-power) but this is inaudible at 20ms durations.

### soft-bypass-smoothing-time: Default smoothing time and configurability

The default SmoothingTime is 20ms, set as a NodeProperty on each container.soft_bypass child (visible in the base JSON: "SmoothingTime": 20). This is user-configurable per soft_bypass container in the scriptnode UI. The valid range is 0-1000ms. Setting 0ms makes it effectively a hard bypass.

### xfader-switch-mode: Does Switch mode send binary 0/1 values?

Yes. In Switch mode, the control.xfader divides its normalized input range into N equal segments (where N = NumParameters = number of switch targets). For each integer step of the Value parameter, exactly one SwitchTarget receives the "active" signal (1.0) while all others receive "inactive" (0.0). The Value parameter on the xfader has range 0.0-1.0 with no step (continuous), but the Switch parameter exposed at the root has range 0-1 with step=1, ensuring integer selection.

### description-accuracy: Generic description is inaccurate

The description "A container for serial processing of nodes" is inherited from the root container.chain factory path and does not describe the template's purpose. A more accurate description would be: "Click-free 2-way signal path switch using soft-bypassed containers." This is flagged in the notes below.

### dummy-placeholder: math.mul dummy is a user insertion point

Confirmed. Each soft_bypass container holds a math.mul node named "dummy" with Value=1.0 (unity gain, transparent pass-through). These are placeholders. Users are expected to replace these dummy nodes with their actual processing chains. The dummy nodes are colored white (Colours::white) to visually distinguish them as placeholders.

## Parameter Routing

| Exposed Parameter | Target Node | Target Parameter | Connection |
|---|---|---|---|
| Switch (0-1, step 1) | control.xfader "switcher" | Value | Direct macro connection |

The xfader internally routes its Value to the SwitchTargets:
- SwitchTarget[0] -> sb1.Bypassed (DynamicBypassParameter)
- SwitchTarget[1] -> sb2.Bypassed (DynamicBypassParameter)

## User Insertion Points

| Container | Dummy Node | Purpose |
|---|---|---|
| sb1 (container.soft_bypass) | math.mul "dummy" (Value=1.0) | Replace with processing chain for Switch=0 |
| sb2 (container.soft_bypass) | math.mul "dummy" (Value=1.0) | Replace with processing chain for Switch=1 |

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors:
- The active soft_bypass child's contents determine the real CPU cost
- During crossfade transitions (default 20ms), both the outgoing and incoming paths process simultaneously, briefly doubling the processing load
- The xfader control node itself is negligible (outside signal path)

## Notes

- The description in scriptnodeList.json is the generic container.chain description and should read something like "Click-free 2-way signal path switch using soft-bypassed containers."
- This template provides the same functionality as container.branch but with click-free transitions. container.branch hard-switches (instant, may click); this template soft-switches (crossfaded, click-free).
- The sb_container uses IsVertical=false for horizontal layout of the soft_bypass children, while the root chain uses IsVertical=true.
- All soft_bypass containers and the xfader share the same NodeColour (4288243590) for visual grouping. Dummy nodes are white.
