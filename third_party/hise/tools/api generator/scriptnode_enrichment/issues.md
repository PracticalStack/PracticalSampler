# Scriptnode Enrichment -- Issues Found During Source Analysis

## Issue 1: DataReadLock default behavior is try-read, not blocking read

**File:** `hi_dsp_library/snex_basics/snex_ExternalData.h`, line 833-835

The `DataReadLock` constructor has a `tryRead` parameter that defaults to `false`.
However, when `tryRead` is false, it calls `enterTryReadLock()` (non-blocking).
When `tryRead` is true, it calls `enterReadLock()` (blocking). This is
counter-intuitive -- the parameter name and default suggest "don't try, just
lock", but the implementation does the opposite.

```cpp
if (tryRead)
    holdsLock = lockToUse->enterReadLock();      // blocking
else
    holdsLock = lockToUse->enterTryReadLock();   // non-blocking (default)
```

This means the default `DataReadLock(base*)` is actually a try-lock that may
fail silently. The `operator bool()` check is essential but easy to forget.

**Impact:** Nodes that use `DataReadLock` without checking `isLocked()` may
read stale or inconsistent data if the lock was not acquired. This is likely
intentional for audio-thread safety (never block), but the naming is misleading.

## Issue 2: dyn<float> operator= copies data twice

**File:** `hi_dsp_library/snex_basics/snex_ArrayTypes.h`, lines 609-613

The `operator=(OtherContainer&)` method contains a redundant memcpy:

```cpp
int n = jmin(size(), other.size());
memcpy(begin(), other.begin(), n * sizeof(T));    // first copy (bounded)
memcpy(begin(), other.begin(), size() * sizeof(T)); // second copy (full size)
```

The second memcpy overwrites the first and uses `size()` instead of the bounded
`n`. If `other.size() < size()`, the second memcpy reads past the end of `other`.

**Impact:** Potential out-of-bounds read when source container is smaller than
destination. The jasserts above should catch size mismatches in debug builds.

## Issue 3: math.clip opSingle multiplies instead of clamping

**File:** `hi_dsp_library/dsp_nodes/MathNodes.h`, line 410

The frame-processing path for `math::Operations::clip` contains:
```cpp
s *= jlimit(-value, value, s);
```

This multiplies `s` by the clamped value of `s`, producing `s * clamp(s, -value, value)`.
The block-processing path (line 403) correctly uses `hmath::vclip(b, -value, value)`
which is a pure clamp operation.

**Impact:** Frame-based processing (e.g., inside `fix_block` containers with
blockSize=1, or frame-based container nodes) produces different output than
block-based processing for the same `math.clip` node. The frame path applies a
nonlinear distortion instead of hard clipping. For signals within [-value, value],
the result is `s * s` (squaring), not `s` (identity).

## Issue 4: math.div silently zeroes signal for negative Value parameter

**File:** `hi_dsp_library/dsp_nodes/MathNodes.h`, lines 254, 266

The `math::Operations::div` node checks `value > 0.0f` and sets the factor to
0 for non-positive divisors:
```cpp
auto factor = value > 0.0f ? 1.0f / value : 0.0f;
```

**Impact:** Setting the Value parameter to any negative number produces silence
instead of dividing by a negative value. This is asymmetric with `math.mul`
which works with both positive and negative values. Users expecting `div(-2)` to
invert phase and halve amplitude will get silence instead.

## Issue 5: hmath::sign() inconsistency at zero between float and double

**File:** `hi_dsp_library/snex_basics/snex_Math.h`, lines 149, 169

The double overload uses `value >= 0.0` (returns +1 for zero).
The float overload uses `value > 0.0f` (returns -1 for zero).

```cpp
// double: sign(0.0) = (0.0 >= 0.0) * 2.0 - 1.0 = 1.0
static constexpr double sign(double value) { return (double)(value >= 0.0) * 2.0 - 1.0; };
// float: sign(0.0f) = -1.0f (because 0.0f is not > 0.0f)
static constexpr float sign(float value) { return value > 0.0f ? 1.0f : -1.0f; };
```

**Impact:** `hmath::abs(0.0f)` returns `-0.0f` (negative zero) because abs is
defined as `value * sign(value)` = `0.0f * -1.0f`. This is functionally
equivalent to `0.0f` in IEEE 754 arithmetic but could cause unexpected results
in sign-bit-sensitive comparisons or serialization.

## Issue 6: Clone sanity check has duplicate condition branch

**File:** `hi_snex/snex_cpp_builder/snex_jit_ValueTreeBuilder.cpp`, lines 976-982

The clone container export validation has two branches that check the same
condition with identical logic:

```cpp
if(numStaticClones == -1 && numStaticCableClones != -1)
{
    e.errorMessage << "\n> `Container.NumClones` is automated but `" << cableId << ".NumClones` is static.";
}
else if (numStaticClones == -1 && numStaticCableClones != -1)  // same condition
{
    e.errorMessage << "\n> `Container.NumClones` is static but `" << cableId << ".NumClones` is automated.";
}
```

The second branch (line 980) should check `numStaticClones != -1 && numStaticCableClones == -1`
(the inverse condition). As written, the "static container + automated cable"
case falls through to the generic mismatch message instead of getting the
specific error text.

**Impact:** Misleading error message when a clone container has static NumClones
but a connected clone cable has automated NumClones. The error still fires
(caught by the earlier `numStaticClones != numStaticCableClones` check) but
shows the generic "NumClones mismatch" message instead of the specific one.

## Issue 7: SplitNode::handleHiseEvent passes original instead of copy

**File:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.cpp`, lines 119-129

The `SplitNode::handleHiseEvent()` creates a copy of the HiseEvent but then
passes the original reference to each child, leaving the copy unused:

```cpp
void SplitNode::handleHiseEvent(HiseEvent& e)
{
    if (isBypassed())
        return;

    for (auto n : nodes)
    {
        HiseEvent copy(e);       // copy created but never used
        n->handleHiseEvent(e);   // should be: n->handleHiseEvent(copy);
    }
}
```

Line 127 should pass `copy` instead of `e`. As written, if the first child
modifies the event (e.g., changes velocity or channel), subsequent children
see the modified version. This makes split event handling behave identically
to chain (serial) event handling, which is likely unintended.

Compare with `MultiChannelNode::handleHiseEvent()` (lines 550-556) which
correctly creates a copy `c` and passes `c` to each child. Also compare with
the compiled `container::split` which passes the same reference by design
(since compiled nodes typically do not modify events in handleHiseEvent).

**Impact:** Low in practice -- most nodes do not modify HiseEvent in
handleHiseEvent(). But any node that does (e.g., transposing, channel
filtering) would cause unexpected cross-talk between split branches.

## Issue 8: wrap::repitch hardcoded to 1 or 2 channels only

**File:** `hi_dsp_library/node_api/nodes/processors.h`, lines 880-901

The `repitch::process()` method dispatches only for 1 or 2 channels:

```cpp
if(data.getNumChannels() == 1)
    processFixed<1>(data);
if(data.getNumChannels() == 2)
    processFixed<2>(data);
```

For channel counts > 2, no processing occurs -- the data passes through
unmodified. There is no assertion or error for unsupported channel counts,
so the failure is silent.

**Impact:** A `container.repitch` inside a `container.fix4_block` (or any
context with > 2 channels) silently does nothing. The child nodes still
process at the original sample rate despite the repitch wrapper being present.

## Issue 9: wrap::dynamic_blocksize has unused static_functions members

**File:** `hi_dsp_library/node_api/nodes/processors.h`, lines 1395-1401

The `dynamic_blocksize` class declares seven `static_functions::fix_block<N>`
member variables (b8, b16, b32, b64, b128, b256, b512) that are never used.
The actual processing dispatches via the BLOCKSIZE_CASE macro which calls
`static_functions::fix_block<N>::process()` as a static function, not through
these member instances.

```cpp
static_functions::fix_block<8> b8;    // never used
static_functions::fix_block<16> b16;  // never used
// ... etc
```

**Impact:** No functional impact. These are empty structs (only contain a
static `SN_EMPTY_INITIALISE` macro) so the memory overhead is negligible.
Likely leftover from an earlier implementation that used member instances.

## Issue 10: clone_forward::createParameters uses clone_cable's DEFINE_PARAMETERDATA

**File:** `hi_dsp_library/dsp_nodes/CableNodes.h`, lines 1685-1691

The `clone_forward::createParameters()` method uses `DEFINE_PARAMETERDATA(clone_cable, ...)`
instead of `DEFINE_PARAMETERDATA(clone_forward, ...)`:

```cpp
// Inside clone_forward::createParameters():
DEFINE_PARAMETERDATA(clone_cable, NumClones);  // should be clone_forward
// ...
DEFINE_PARAMETERDATA(clone_cable, Value);      // should be clone_forward
```

The `DEFINE_PARAMETERDATA` macro uses its first argument to generate the
`setParameterStatic` callback binding. Since `clone_forward` defines its own
`setParameterStatic` via `DEFINE_PARAMETERS` / `SN_PARAMETER_MEMBER_FUNCTION`,
the callback should point to `clone_forward::setParameterStatic`, not
`clone_cable::setParameterStatic`.

**Impact:** In the compiled (exported) path, parameter callbacks are wired at
compile time via template connections, so this macro output is not used. In the
interpreted path, `OpaqueNode::create()` calls `createParameters()` and the
generated `parameter::data` objects carry function pointers. If the macro
resolves to `clone_cable::setParameterStatic`, setting NumClones or Value would
call clone_cable's parameter dispatch instead of clone_forward's. However,
since clone_forward is always instantiated with `parameter::clone_holder`
(dynamic), and the dynamic path uses `clone_holder::callEachClone` directly
rather than these static callbacks, the bug may be masked at runtime.

## Issue 11: filters.svf Allpass mode inconsistency between process and processFrame

- **Type:** inconsistency
- **Severity:** high
- **Location:** `hi_dsp_library/dsp_basics/MultiChannelFilters.cpp`:1075 vs 1173
- **Observed:** In `StateVariableFilterSubType::processSamples()` (block path, line 1075), the HP term for Allpass mode is computed as `(input - x1*z1_A - v2) * x2` (multiply by x2). In `processFrame()` (line 1173), the same HP term is computed as `(input - x1*z1_A - v2) / x2` (divide by x2). Since `x2 = 1/(1 + 2*R*g + g*g)`, multiply and divide by x2 produce different results.
- **Expected:** Both paths should use the same operation. The block path (multiply by x2) matches the standard Zavalishin TPT SVF formulation and is likely correct. The frame path (divide by x2) appears to be a bug.

## Issue 12: filters.moog Mode parameter is display-only, does not affect processing

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_basics/MultiChannelFilters.cpp`:371-374, 389-412
- **Observed:** `MoogFilterSubType::setType(int /*t*/)` is empty (line 371-374). The Mode parameter (values: "One Pole", "Two Poles", "Four Poles") only affects the filter display curve via `getCoefficientTypeList()`. The `processSamples()` and `processFrame()` methods always run the full 4-pole ladder processing and output from stage 4. The `mode` member variable has a `setMode()` setter that is never called from the FilterNodeBase path (which calls `setType()`, not `setMode()`).
- **Expected:** Either the Mode parameter should switch between 1-pole, 2-pole, and 4-pole outputs (by outputting from out1, out2, or out4 respectively), or the Mode parameter should be removed/hidden and the node documented as always being 4-pole.

## Issue 13: LinkwitzRiley::processSamples incorrect channel pointer and ignores numSamples

- **Type:** silent-fail
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_basics/MultiChannelFilters.cpp`:1242-1253
- **Observed:** `processSamples(AudioSampleBuffer& buffer, int startSample, int)` has two issues: (1) `buffer.getWritePointer(c + startSample)` passes `c + startSample` as the channel index instead of `buffer.getWritePointer(c, startSample)`. (2) The numSamples parameter is unnamed/ignored; the loop uses `buffer.getNumSamples()` instead. In the scriptnode path, startSample is always 0 (set by FilterNodeBase::process), which mitigates the channel index bug, and the buffer size matches numSamples, which mitigates the loop count issue.
- **Expected:** `auto ptr = buffer.getWritePointer(c, startSample);` and loop should use the numSamples parameter: `for (int i = 0; i < numSamples; i++)`.

## Issue 14: fx.reverb setWidth() sets damping instead of width

- **Type:** inconsistency
- **Severity:** high
- **Location:** `hi_dsp_library/dsp_nodes/FXNodes.cpp`:93-98
- **Observed:** `reverb::setWidth(double width)` sets `p.damping = jlimit(0.0f, 1.0f, (float)width)` instead of `p.width`. This is a copy-paste error from `setDamping()`. The Width parameter has no effect on stereo width -- it modifies damping a second time. The actual `juce::Reverb::Parameters::width` field is never set by the node and remains at the JUCE default value.
- **Expected:** `p.width = jlimit(0.0f, 1.0f, (float)width);`

## Issue 15: fx.sampleandhold existing phase3 doc has wrong parameter name

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `phase3/fx/sampleandhold.md`
- **Observed:** The existing phase3 documentation lists "Position: The position in the stereo field" as the parameter for fx.sampleandhold. The actual parameter is "Counter" (range 1-64, integer). This is a copy-paste error from the fx.haas documentation.
- **Expected:** Parameter should be documented as "Counter" with its correct description (decimation factor for sample rate reduction).

## Issue 16: container.dynamic_blocksize double prepare() call

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.cpp`:1563-1576
- **Observed:** `DynamicBlockSizeNode::prepare()` calls `obj.prepare(ps)` twice. Lines 1570-1573 call it conditionally (bypassed vs active), then line 1575 calls it unconditionally. The second call always runs with the original PrepareSpecs regardless of bypass state, overriding the conditional logic.
- **Expected:** The unconditional `obj.prepare(ps)` on line 1575 should be removed. The conditional block on lines 1570-1573 already handles both cases correctly.

## Issue 17: container.dynamic_blocksize vestigial fix_block members

- **Type:** vestigial
- **Severity:** low
- **Location:** `hi_dsp_library/node_api/nodes/processors.h`:1395-1401
- **Observed:** `wrap::dynamic_blocksize<T>` declares seven `static_functions::fix_block<N>` members (b8, b16, b32, b64, b128, b256, b512) that are never referenced. The `BLOCKSIZE_CASE` macro on lines 1339-1362 calls `static_functions::fix_block<N>::process()` as static functions, not through these members.
- **Expected:** These members can be removed with no behavioral change.

## Issue 18: container.fix_blockx validation allows values with no switch case

- **Type:** silent-fail
- **Severity:** low
- **Location:** `hi_scripting/scripting/scriptnode/nodes/NodeContainerTypes.h`:665-677
- **Observed:** `DynamicBlockProperty::updateBlockSize()` validates `blockSize > 7 && isPowerOfTwo(blockSize)`, which accepts values like 1024, 2048, etc. However, the process dispatch switch (lines 688-698) only has cases for 8-512. Values >= 1024 fall through the switch without processing any audio.
- **Expected:** Either add an upper bound to the validation (`blockSize <= 512`) or add a default case to the switch. In practice, the UI combobox only offers 8-256, so this is only reachable via direct ValueTree manipulation.

## Issue 19: container.framex_block description missing channel flexibility

- **Type:** inconsistency
- **Severity:** medium
- **Location:** scriptnodeList.json entry for `framex_block`
- **Observed:** The description "Enables per sample processing for the child nodes." does not distinguish framex_block from frame1_block/frame2_block. The key feature -- dynamic channel count adaptation -- is not mentioned.
- **Expected:** Description should note the dynamic channel count, e.g., "Per sample processing that adapts to the current channel count."

## Issue 20: container.sidechain description grammar error

- **Type:** inconsistency
- **Severity:** low
- **Location:** scriptnodeList.json entry for `sidechain`
- **Observed:** Description reads "Creates a empty audio by duplicating the channel amount for sidechain routing." -- "a empty" should be "an empty". The phrasing is also unclear about what is actually created.
- **Expected:** "Doubles the channel count by adding empty sidechain channels."

## Issue 21: container.offline has empty description

- **Type:** inconsistency
- **Severity:** medium
- **Location:** scriptnodeList.json entry for `offline`
- **Observed:** The description field is an empty string. No description at all for this container node.
- **Expected:** "A container for offline (non-realtime) processing that skips the realtime audio callback."

## Issue 22: container.clone polyphonic compilation failure

- **Type:** compile-error
- **Severity:** high
- **Location:** `Containers.h:116` (compiled path)
- **Observed:** Compiling a polyphonic clone container triggers `clone_base has no member named isPolyphonic()`. Reported by multiple users (Feb 2025) on both Windows and macOS. One workaround: ensure the clone container holds at least one non-trivial processing node and that NumClones is set as the first macro control.
- **Expected:** The compiled clone container should support polyphonic contexts or produce a clear error at the network level.
- **Source:** Forum topic 12012

## Issue 23: container.clone with Linkwitz-Riley filters produces silence

- **Type:** silent-fail
- **Severity:** medium
- **Location:** Clone container + filter node interaction
- **Observed:** Placing Linkwitz-Riley filter nodes inside a clone container compiles successfully but produces silence at runtime. Removing the clone wrapper and using the filters directly restores audio. May be a per-clone filter state initialisation issue.
- **Expected:** Filter nodes should produce output when placed inside a clone container.
- **Source:** Forum topic 12012

## Issue 24: container.soft_bypass inside container.clone causes crash

- **Type:** crash
- **Severity:** high
- **Location:** Clone iteration + SoftBypass::process interaction
- **Observed:** Placing a `container.soft_bypass` node around an oscillator inside a `container.clone` crashes HISE. Reported in 2024. The interaction between the clone container's per-clone processing and the soft_bypass state management is the likely cause.
- **Expected:** soft_bypass should work safely inside clone containers.
- **Source:** Forum topic 9277

## Issue 25: Delay time not recalculated on block size or oversampling change

- **Type:** stale-state
- **Severity:** medium
- **Location:** Delay node prepareToPlay / sampleRateChanged callback
- **Observed:** Changing the host block size or oversampling factor does not trigger a recalculation of delay node timing. The delay time parameter retains its previously computed internal value, making the actual delay incorrect relative to the displayed millisecond value. The output shifts permanently after an oversampling round-trip (none -> 2x -> none does not return to original). Moving the delay time parameter manually forces recalculation.
- **Expected:** Delay time should recalculate automatically when block size or sample rate changes.
- **Source:** Forum topic 10420

## Issue 26: Stale modulation connection after deleting source node in modchain

- **Type:** stale-state
- **Severity:** medium
- **Location:** Modulation connection cleanup (IDE behaviour)
- **Observed:** When a node inside a modchain is deleted, its modulation connection can persist on the destination node. The target parameter behaves as if still modulated - it ignores manual input. Fix: manually remove the connection on the target node.
- **Expected:** Deleting a modulation source should automatically clean up all its outgoing connections.
- **Source:** Forum topic 4978

## Issue 27: routing.selector createParameters uses wrong class name in DEFINE_PARAMETERDATA

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/RoutingNodes.h`:1051-1076
- **Observed:** `selector::createParameters()` uses `DEFINE_PARAMETERDATA(receive, ChannelIndex)`, `DEFINE_PARAMETERDATA(receive, NumChannels)`, etc. -- all four parameters reference the `receive` class instead of `selector`. The DEFINE_PARAMETERDATA macro generates parameter callback bindings using the first argument as the class name.
- **Expected:** Should use `DEFINE_PARAMETERDATA(selector, ChannelIndex)` etc. In practice this may be masked because the dynamic parameter path uses `DEFINE_PARAMETERS` / `SN_PARAMETER_MEMBER_FUNCTION` which correctly binds to `selector::setParameterStatic`.

## Issue 28: routing.selector SelectOutput parameter range mismatch

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/RoutingNodes.h`:1066
- **Observed:** The SelectOutput parameter's range is set to `{1.0, 16.0, 1.0}` which defines a range of 1 to 16 with step 1. However, SelectOutput is used as a boolean toggle (compared with `v > 0.5` in `setSelectOutput()`). The `setParameterValueNames({"Disabled", "Enabled"})` call overrides the visual presentation to show two options, but the underlying range does not match the 0/1 boolean semantics.
- **Expected:** Range should be `{0.0, 1.0, 1.0}` to match the boolean usage.

## Issue 29: routing.global_receive -- description copy-paste error

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.cpp`:1642
- **Observed:** `GlobalReceiveNode::getNodeDescription()` returns "Send the signal anywhere in HISE!" -- identical to GlobalSendNode. This is a copy-paste error; the receive node should say "Receive" not "Send".
- **Expected:** Description should be "Receive a signal sent from a global_send node anywhere in HISE" or similar.

## Issue 30: routing.global_receive -- base data isPolyphonic is false

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_scripting/scripting/scriptnode/dynamic_elements/GlobalRoutingManager.cpp`:1577
- **Observed:** The preliminary JSON for routing.global_receive has `isPolyphonic: false`, but the node is registered via `registerPolyNodeRaw<GlobalReceiveNode<1>, GlobalReceiveNode<NUM_POLYPHONIC_VOICES>>()` and has per-voice state (`PolyData<float, NumVoices> value`, `PolyData<int, NumVoices> offset`). The polyphonic variant handles per-voice offset on note-on.
- **Expected:** Base data classification should indicate polyphonic support exists (AllowPolyphonic or isPolyphonic depending on the variant).

## Issue 31: routing.event_data_reader -- static mode ModValue shared across voices

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/RoutingNodes.h`:510
- **Observed:** In static mode, `ModValue staticValue` is a single member (not wrapped in PolyData). When multiple polyphonic voices trigger note-on, each overwrites the same staticValue. This means in polyphonic static mode, the modulation output reflects the last voice that started, not necessarily the current voice's value.
- **Expected:** For correct polyphonic static mode, `staticValue` should be `PolyData<ModValue, NV>` or the static read should use a per-voice cache. In dynamic mode, this is not an issue because the per-voice eventId correctly looks up per-voice data.

## Issue 32: control.compare description inaccurate for MIN/MAX modes

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h`:2231
- **Observed:** The SN_DESCRIPTION says "compares the input signals and outputs either 1.0 or 0.0". However, the MIN (index 6) and MAX (index 7) comparator modes return `jmin(leftValue, rightValue)` and `jmax(leftValue, rightValue)` respectively -- continuous values, not binary 0/1. Only EQ through LET (indices 0-5) produce strict 0/1 output.
- **Expected:** Description should acknowledge MIN/MAX modes produce continuous output, e.g., "Compares input signals and outputs 1.0 or 0.0 (or min/max of inputs for MIN/MAX modes)."

## Issue 33: control.blend preliminary JSON incorrectly reports normalised output

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `scriptnode_enrichment/preliminary/control.blend.json`:63
- **Observed:** The preliminary JSON has `modulationOutput.isUnnormalised: false`, but `multilogic::blend::isNormalisedModulation()` returns `false` at `CableNodes.h:2055`. This means the output IS unnormalised. The control-infrastructure.md correctly lists blend as unnormalised.
- **Expected:** `modulationOutput.isUnnormalised` should be `true` in the preliminary JSON.

## Issue 34: pack_resizer -- vestigial float member

- **Type:** vestigial
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h`:854
- **Observed:** `pack_resizer` has a member `float something = 90.0f` that is never read or written by any method. It is not referenced in any processing, parameter, or serialization code.
- **Expected:** The field can be removed with no behavioral change.

## Issue 35: pack_resizer -- NumSliders parameter range inconsistent with implementation clamping

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h`:837,846
- **Observed:** The NumSliders parameter is defined with range `{0.0, 128.0, 1.0}` (line 846), allowing a minimum of 0. However, `setParameter<0>()` clamps to `jlimit<int>(1, 128, ...)` (line 837), making 0 effectively impossible. The parameter default is 0.0 which gets clamped to 1.
- **Expected:** Parameter range minimum should be 1.0 to match the implementation: `{1.0, 128.0, 1.0}`.

## Issue 36: clone_forward and clone_cable -- numClones member not initialized

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h`:1600,1699
- **Observed:** Both `clone_cable` (line 1600) and `clone_forward` (line 1699) declare `int numClones;` without initialization. Both rely on `setNumClones()` being called before `sendValue()`, but if `setValue()` is called before `setNumClones()`, the uninitialized `numClones` could cause undefined loop bounds in `sendValue()`.
- **Expected:** `int numClones = 1;` (matching the parameter default).

## Issue 34: control.blend Value1/Value2 not registered as unscaled despite unnormalised output

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h:2081-2101`
- **Observed:** The `multilogic::blend` class does NOT inherit from `no_mod_normalisation` and does not register Value1 or Value2 as unscaled parameters. However, `isNormalisedModulation()` returns false, making the output unnormalised. Input values are scaled by the connection system but the output bypasses range conversion.
- **Expected:** If blend is intended to work with raw values, Value1 and Value2 should be registered as unscaled via `no_mod_normalisation`.

## Issue 35: control.cable_expr lastValue member is vestigial

- **Type:** vestigial
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h:1096`
- **Observed:** The `cable_expr` class declares `double lastValue = 0.0` but never reads or writes to it in any method.
- **Expected:** The `lastValue` member can be removed with no behavioral change.

## Issue 36: control.midi_cc EnableMPE flag is vestigial

- **Type:** vestigial
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h:1044-1059`
- **Observed:** The `enableMpe` boolean is set by `setEnableMPE(double)` but is never read in `handleHiseEvent()` or any other method. The `isInPolyphonicContext` member is also declared but unused.
- **Expected:** EnableMPE should filter MIDI CC events by MPE zone channels, or the parameter should be removed if MPE support is not implemented.

## Issue 37: control.voice_bang missing IsProcessingHiseEvent property

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h:1099`
- **Observed:** voice_bang implements `handleHiseEvent()` and responds to note-on events, but does not register `IsProcessingHiseEvent` as a CustomNodeProperty. The method works because OpaqueNode::create wires function pointers based on method existence, but the missing property may cause confusion in the C++ code generator or documentation.
- **Expected:** voice_bang should register `IsProcessingHiseEvent` to match its actual behaviour.

## Issue 38: control.midi_cc missing IsProcessingHiseEvent property

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/CableNodes.h:951`
- **Observed:** midi_cc implements `handleHiseEvent()` and processes MIDI CC/pitchbend/aftertouch/note events, but does not register `IsProcessingHiseEvent`. It inherits `SN_EMPTY_HANDLE_EVENT` from `no_processing` but overrides it in the class body.
- **Expected:** midi_cc should register `IsProcessingHiseEvent` to match its actual behaviour.

### core.faust -- empty description in base data

- **Type:** inconsistency
- **Severity:** medium
- **Location:** scriptnodeList.json (core.faust entry)
- **Observed:** The description field is an empty string for core.faust. The C++ source does not define an SN_DESCRIPTION macro for faust_jit_node_base.
- **Expected:** A description such as "A Faust JIT node that compiles and runs Faust DSP code" should be present.

### core.stretch_player -- SN_EMPTY_HANDLE_EVENT despite IsProcessingHiseEvent

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/StretchNode.h:217`
- **Observed:** stretch_player declares `SN_EMPTY_HANDLE_EVENT` but inherits from `polyphonic_base` which registers `IsProcessingHiseEvent`. The node does not actually process MIDI events in its handleHiseEvent callback. The Gate parameter must be driven externally rather than by MIDI note-on/note-off.
- **Expected:** Either the node should handle MIDI events to trigger playback (like file_player), or the IsProcessingHiseEvent flag registration should be suppressed via polyphonic_base constructor parameter (`addProcessEventFlag=false`).

### core.stretch_player -- Pitch parameter clamped wider than range

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/StretchNode.h:420`
- **Observed:** The Pitch parameter range in createParameters is -12 to +12, but setParameter<2> clamps to -24 to +24 (`jlimit(-24.0, 24.0, v)`). Modulation could push the pitch beyond the UI-visible range.
- **Expected:** Either the clamp should match the parameter range (-12 to +12), or the parameter range should be documented as extendable via modulation.

---

## Modulation Bridge Nodes (core.global_mod, core.extra_mod, core.pitch_mod, core.matrix_mod)

### core.global_mod -- Missing SN_DESCRIPTION macro

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:570`
- **Observed:** global_mod has no `SN_DESCRIPTION()` macro, unlike pitch_mod and matrix_mod which both define descriptions.
- **Expected:** A description string should be provided for consistency, e.g. "Picks up a modulation signal from the GlobalModulatorContainer".

### core.extra_mod -- Missing SN_DESCRIPTION macro

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:646`
- **Observed:** extra_mod has no `SN_DESCRIPTION()` macro.
- **Expected:** A description string should be provided, e.g. "Picks up a modulation signal from an extra modulation chain of a hardcoded effect".

### core.matrix_mod -- SourceIndex/AuxIndex range discrepancy

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:949`
- **Observed:** The C++ createParameters sets the range to `{-1.0, (double)modulation::NumMaxModulationSources, 1.0}` where NumMaxModulationSources depends on HISE_NUM_MODULATORS_PER_CHAIN (default 128). The preliminary JSON shows max 64. The actual runtime max depends on the preprocessor define.
- **Expected:** The JSON should reflect the actual C++ range, or the range should be documented as build-dependent.

### core.matrix_mod -- No output clamping

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/ModulationNodes.h:844-885`
- **Observed:** matrix_mod's `applyModulation()` does not clamp output to [0,1] for any mode (Scale, Unipolar, Bipolar). This differs from global_mod's mod_base `applyModulation()` which clamps all Gain/Unipolar/Bipolar outputs to [0,1].
- **Expected:** Either intentional design difference (matrix_mod allows out-of-range values by design) or should match global_mod's clamping behavior. Documentation should clarify.

## Issue 39: core.table -- audio waveshaping is broken (passthrough only)

- **Type:** silent-fail
- **Severity:** high
- **Location:** `hi_dsp_library/dsp_nodes/CoreNodes.h`:82-101 (process), 115-131 (processFrame)
- **Observed:** The `table::process()` method iterates samples with `ignoreUnused(s)` and operates on a local variable `v` instead of the actual sample `s`. The `processFloat(v)` call modifies `v` in-place but the result is never written back to the audio buffer. Similarly, `processFrame()` reads `v = hmath::abs(s)` and processes `v` through the table but never writes back to `s` or `data`. The audio signal passes through completely unmodified. Additionally, the `ModValue currentValue` is only set in `reset()` (to 0.0) and is never updated during processing, making the modulation output non-functional.
- **Expected:** Based on the SN_DESCRIPTION "a (symmetrical) lookup table based waveshaper", the node should modify the audio signal by applying the table lookup to each sample. Either `s = processFloat(s)` or the modulation output should be properly updated with the table lookup result.

## Issue 40: core.fm -- preliminary JSON incorrectly classifies as monophonic

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `scriptnode_enrichment/preliminary/core.fm.json`:10-11
- **Observed:** The preliminary JSON has `isPolyphonic: false` and `isProcessingHiseEvent: false`. However, the C++ source at CoreNodes.h:2038-2041 declares `isProcessingHiseEvent() = true` and `isPolyphonic() = true`. The node uses `PolyData<OscData, NUM_POLYPHONIC_VOICES>` and `PolyData<double, NUM_POLYPHONIC_VOICES>`. The `handleHiseEvent()` in CoreNodes.cpp:98 responds to note-on for frequency tracking.
- **Expected:** Preliminary JSON should have `isPolyphonic: true` and `isProcessingHiseEvent: true`.

## Issue 41: core.granulator -- MIDI handling not reflected in preliminary JSON

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `scriptnode_enrichment/preliminary/core.granulator.json`:10
- **Observed:** The preliminary JSON has `isProcessingHiseEvent: false`. However, the C++ source at CoreNodes.h:2734 implements `handleHiseEvent()` which processes note-on, note-off, CC#64 (sustain pedal), and all-notes-off events. The granulator requires MIDI note-on events to produce sound (its internal voice counter must be > 0).
- **Expected:** The node should be flagged with isProcessingHiseEvent. Note: the SNEX_NODE macro does not register this property automatically, so it may genuinely be missing from the node's runtime properties, which would mean MIDI events are never forwarded to it.

## Issue 42: envelope.silent_killer -- Threshold parameter is vestigial

- **Type:** vestigial
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`:1813-1816
- **Observed:** The Threshold parameter is converted from dB to linear gain via `Decibels::decibelsToGain()` and stored in the `threshold` member variable. However, the `process()` method uses `d.isSilent()` which has its own hardcoded threshold (~-90dB). The `threshold` member is never read after being set.
- **Expected:** Either the Threshold parameter should be used in the silence check (e.g., comparing peak level against threshold), or the parameter should be documented as non-functional. The parameter range (-120 to -60 dB) suggests intentional configurability.

## Issue 43: envelope.silent_killer -- Description grammar error

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`:1765
- **Observed:** Description reads "Send a voice reset message as soon when silence is detected"
- **Expected:** "Send a voice reset message as soon as silence is detected" or "Send a voice reset message when silence is detected"

## Issue 44: envelope.global_mod_gate / extra_mod_gate -- Description grammar error

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`:1977, 1996
- **Observed:** Both descriptions read "Sends a On-Off modulation signal..."
- **Expected:** "Sends an On-Off modulation signal..."

## Issue 45: envelope.silent_killer -- IsProcessingHiseEvent mismatch

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`:1778-1810
- **Observed:** The `polyphonic_base` constructor is called with `addProcessEventFlag=false`, so `IsProcessingHiseEvent` is NOT registered. However, `handleHiseEvent()` is defined at line 1805 and tracks note-on/note-off state. The preliminary JSON has `isProcessingHiseEvent: false`. If events are not forwarded, the per-voice boolean state will never be set to true on note-on, meaning silence detection could kill voices prematurely (since `state.get()` defaults to false, and the condition `!s` would always be true).
- **Expected:** Either `IsProcessingHiseEvent` should be registered (change `false` to `true` in the polyphonic_base constructor call), or the node relies on being placed inside a `wrap::event` container that forwards events.

## Issue 46: envelope.voice_manager -- Unreferenced parameter slot P==1

- **Type:** vestigial
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/EnvelopeNodes.h`:2038-2039
- **Observed:** `setParameter<>()` handles P==1 with `p->sendVoiceResetMessage(true)` (all-voices panic), but only parameter 0 ("Kill Voice") is registered in `createParameters()`. Parameter slot 1 is never accessible through the normal parameter system.
- **Expected:** Either remove the P==1 handler or register a second parameter. This may be intentional internal API for the editor's panic button.

## Issue 42: math.clip -- opSingle multiplies by clamped value instead of clamping

- **Type:** silent-fail
- **Severity:** critical
- **Location:** `hi_dsp_library/dsp_nodes/MathNodes.h`:410
- **Observed:** The frame-processing path (`OP_SINGLE`) computes `s *= jlimit(-value, value, s)`, which multiplies the sample by its own clamped value instead of simply clamping it. This produces `s * clamp(s, -value, value)` rather than `clamp(s, -value, value)`. The block path (line 403) correctly uses `hmath::vclip(b, -value, value)` which is a pure clamp.
- **Expected:** The frame path should be `s = jlimit(-value, value, s)` (assignment, not multiplication) to match the block path behaviour.

## Issue 43: math.div -- only guards positive divisors, negative Value produces silence

- **Type:** missing-validation
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/MathNodes.h`:254,266
- **Observed:** Both block and frame paths check `value > 0.0f` and set factor to `1.0f / value` if true, else `0.0f`. A negative Value parameter (e.g., -2.0) is treated identically to zero, producing silence. Negative division (signal inversion with scaling) is not supported.
- **Expected:** The guard should check `value != 0.0f` (or `abs(value) > epsilon`) to allow negative divisors, which would produce inverted and scaled output.

## Issue 44: math.pi -- description typo "3.13" instead of 3.14159

- **Type:** inconsistency
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/MathNodes.h`:318
- **Observed:** The SN_DESCRIPTION string reads "Multiplies the signal with PI (3.13)". The actual value used is `hmath::PI` which is 3.14159265358979...
- **Expected:** Description should read "3.14" or "3.14159" instead of "3.13".

## Issue 45: math.sqrt -- NaN on negative input (no abs protection)

- **Type:** silent-fail
- **Severity:** high
- **Location:** `hi_dsp_library/dsp_nodes/MathNodes.h`:445
- **Observed:** The `opSingle` implementation calls `sqrtf(s)` directly without any protection against negative input values. Audio signals are bipolar (typically [-1, 1]), so negative samples will produce NaN. The NaN then propagates through downstream processing.
- **Expected:** Either apply `abs()` before sqrt (i.e., `sqrtf(fabsf(s))`) or clamp input to non-negative range. Alternatively, document that the node expects unipolar [0, 1] input only.

## Issue 46: math.neural -- description says "first channel" but code processes all channels

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/MathNodes.h`:824,957
- **Observed:** The SN_DESCRIPTION reads "Runs a per-sample inference on the first channel of the signal using a neural network". However, the `process()` method at line 957 iterates `for(auto& ch: data)` which processes ALL channels, each with its own network instance (offset + channel index). The `processFrame()` method (line 980) also processes all channels.
- **Expected:** Description should read "all channels" instead of "the first channel".

## Issue 47: dynamics.comp/gate/limiter -- ModulationTargets empty despite active modulation output

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `scriptnodeList.json` (dynamics.comp, dynamics.gate, dynamics.limiter entries)
- **Observed:** The `ModulationTargets` field is empty `{}` for all three nodes. However, the C++ source clearly implements modulation output: `isNormalisedModulation()` returns true, `handleModulation()` returns `modValue` which stores `1.0 - obj.getGainReduction()`. The descriptions also reference "modulation signal".
- **Expected:** `ModulationTargets` should reflect the active modulation output.

## Issue 48: dynamics.envelope_follower -- ModulationTargets empty despite active modulation output

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `scriptnodeList.json` (dynamics.envelope_follower entry)
- **Observed:** `ModulationTargets` is empty. C++ implements `isNormalisedModulation() = true` and `handleModulation()` returning the envelope value.
- **Expected:** `ModulationTargets` should reflect the active modulation output.

## Issue 49: dynamics.envelope_follower -- empty description in base data

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `scriptnodeList.json` (dynamics.envelope_follower entry)
- **Observed:** The `description` field is an empty string. The node does not define `SN_DESCRIPTION` or `getDescription()`.
- **Expected:** Should have a description such as "Tracks input amplitude with per-voice envelope follower, outputs envelope as modulation signal".

## Issue 50: dynamics.updown_comp -- fixed 2-channel processing

- **Type:** ux-issue
- **Severity:** low
- **Location:** `hi_dsp_library/dsp_nodes/DynamicsNode.h`:509
- **Observed:** `getFixChannelAmount()` returns 2. The node casts input to `ProcessData<2>` in `process()`. Mono or multi-channel configurations will not work correctly.
- **Expected:** Users should be informed that this node requires stereo context.

---

### jdsp.jcompressor -- Parameter spelling: "Treshold" missing 'h'

- **Type:** inconsistency
- **Severity:** medium
- **Location:** `hi_dsp_library/dsp_nodes/JuceNodes.cpp` (createParameters)
- **Observed:** Parameter is registered as "Treshold" (missing second 'h'). The underlying JUCE Compressor method is correctly named setThreshold().
- **Expected:** Parameter should be "Threshold" per standard English spelling. This is a cosmetic issue but affects documentation and user-facing parameter names.

### jdsp.jdelay -- Phase3 doc has incorrect frontmatter

- **Type:** inconsistency
- **Severity:** low
- **Location:** `scriptnode_enrichment/phase3/jdsp/jdelay.md`
- **Observed:** The frontmatter of jdelay.md says `keywords: jdelay_thiran` and `summary: A interpolating delay line using the Thiran interpolation algorithm`. This describes jdelay_thiran, not jdelay (linear interpolation).
- **Expected:** Frontmatter should reference jdelay and linear interpolation.

### analyse.oscilloscope -- IsProcessingHiseEvent flag mismatch

- **Type:** inconsistency
- **Severity:** high
- **Location:** scriptnode_enrichment/preliminary/analyse.oscilloscope.json:12
- **Observed:** cppProperties does not include "IsProcessingHiseEvent": true, but the C++ code (line 445 in AnalyserNodes.h) returns true from isProcessingHiseEvent() constexpr for Helpers::Oscilloscope specialization. The node actively processes MIDI note-on events in handleHiseEvent() (lines 467-475) to enable cycle-sync.
- **Expected:** cppProperties should include "IsProcessingHiseEvent": true to match the C++ behavior and enable MIDI event routing.

### analyse.specs -- Empty description field

- **Type:** inconsistency
- **Severity:** medium
- **Location:** scriptnode_enrichment/preliminary/analyse.specs.json:6
- **Observed:** The description field is empty. The node displays PrepareSpecs information (sample rate, block size, channel count, MIDI status, polyphony status).
- **Expected:** Description should be "Displays processing context (sample rate, block size, channels, MIDI/polyphony status). Debug tool; removed in C++ export."

### analyse.fft -- PropertyObject configuration terminology

- **Type:** ux-issue
- **Severity:** low
- **Location:** scriptnode_enrichment/phase3/analyse/fft.md
- **Observed:** The existing doc is well-structured but does not explicitly note that all nine properties (BufferLength, WindowType, Overlap, DecibelRange, UsePeakDecay, UseDecibelScale, YGamma, Decay, UseLogarithmicFreqAxis) are SimpleRingBuffer PropertyObject properties, not scriptnode parameters. This is important for users to understand where to configure these settings.
- **Expected:** Clarify that FFT configuration happens via the display buffer UI (ring buffer properties panel), not via scriptnode parameter connections.

### analyse.goniometer -- Missing base description

- **Type:** inconsistency
- **Severity:** medium
- **Location:** scriptnode_enrichment/preliminary/analyse.goniometer.json:6
- **Observed:** Existing phase3 doc is a stub (7 lines, tier: STUB). No details on stereo correlation method or channel requirements.
- **Expected:** Exploration provides complete signal flow and method (Lissajous X-Y plot of L/R correlation).

## Issue: Sustain pedal (CC64) ignored inside scriptnode envelope

- **Node:** envelope.ahdsr
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:13641
- **Observed:** The sustain pedal (MIDI CC 64) works with the flex envelope outside scriptnode but is ignored when envelope.ahdsr is used inside a scriptnode network.
- **Expected:** CC64 should extend the sustain phase, matching the behaviour of the module-tree AHDSR envelope.

## Issue: Chain node rendered twice inside frame2_block (fixed May 2022)

- **Node:** container.chain (inside frame2_block)
- **Type:** bug (fixed)
- **Severity:** high
- **Source:** Forum tid:5825
- **Author:** Christoph Hart [author]
- **Observed:** A chain node was rendered twice when used inside a frame2_block context, causing oscillators to run an octave higher and other significant processing differences. container.fixblock nodes were not affected. Compiled-to-C++ networks also worked correctly, which may explain discrepancies between interpreted and compiled networks.
- **Status:** Fixed in May 2022. If relying on pre-fix behaviour, audit existing networks.

## Issue: Compiling an empty or polyphonic clone container fails with isPolyphonic() error

- **Node:** container.clone
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:12012
- **Author:** David Healey [expert], HISEnberg [trusted]
- **Observed:** Compiling a clone container that is empty or contains only core::empty nodes produces a compiler error: "has no member named isPolyphonic". Reproducible on both macOS and Windows.
- **Workaround:** Ensure the clone container has actual processing nodes inside it, and that NumClones is specified as the first macro control before attempting compilation.

## Issue: Wrapping an oscillator in container.soft_bypass inside a clone container crashes

- **Node:** container.clone + container.soft_bypass
- **Type:** bug (crash)
- **Severity:** high
- **Source:** Forum tid:9277
- **Author:** David Healey [expert]
- **Observed:** Placing a container.soft_bypass node around an oscillator node inside a clone container caused a crash. Confirmed with a snippet.
- **Workaround:** Avoid soft_bypass wrappers around oscillator-type nodes within clone containers.

## Issue: Compiled VST3 FX plugin crashed when channel count exceeded 2 (fixed late 2022)

- **Node:** routing.matrix (multichannel context)
- **Type:** bug (fixed)
- **Severity:** high
- **Source:** Forum tid:6685
- **Author:** Christoph Hart [author]
- **Observed:** A commit in late 2022 introduced a crash in compiled multichannel plugins (VST3 and standalone) when the routing matrix used more than 2 channels. The crash occurred at AudioBuffer::setSize.
- **Status:** Fixed the same day. Update to a post-fix develop branch snapshot if building against a snapshot from that period.

## Issue: core.oscillator uptime counter overflow after extended playback

- **Node:** core.oscillator
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:10696
- **Observed:** A core.oscillator running on the second channel inside a modchain context causes the internal uptime counter to overflow after several minutes of continuous playback, producing an audio artefact.
- **Expected:** The uptime counter should wrap or reset gracefully without producing audible artefacts during extended playback.

## Issue: core.granulator crashes with audio files shorter than the grain length

- **Node:** core.granulator
- **Type:** bug
- **Severity:** high
- **Source:** Forum tid:8030
- **Observed:** Loading an audio file that is shorter than the configured grain length into the granulator causes crashes or audio artefacts. The edge case is not handled.
- **Expected:** The granulator should clamp the grain length to the available audio length or report an error gracefully.

## Issue: MatrixPeakMeter maxPeaks array always returns [0, 0]

- **Node:** core.peak (MatrixPeakMeter FloatingTile)
- **Type:** bug
- **Severity:** low
- **Source:** Forum tid:11190
- **Observed:** The `maxPeaks` array in the MatrixPeakMeter LAF callback always contains `[0, 0]` regardless of actual peak levels. Identified as a typo in the source; a PR was submitted.
- **Expected:** The maxPeaks array should report the peak-hold values for each channel.

## Issue: Moog filter produces noise bursts under modulation

- **Node:** filters.moog
- **Type:** bug
- **Severity:** high
- **Source:** Forum tid:7834
- **Observed:** The filters.moog node produces sudden loud noise transients, especially when cutoff or resonance are modulated by LFOs or envelopes. Multiple trusted users report this independently.
- **Expected:** The filter should remain stable under modulation.
- **Workaround:** Use filters.ladder instead, which provides the same 4-pole ladder topology without this instability.
- **Status:** Long-standing issue. Community consensus for several years has been to avoid filters.moog entirely.

## Issue: Convolution output gain increases at sample rates above 48 kHz

- **Node:** filters.convolution
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:2054
- **Observed:** The convolution engine exhibits gain increase at higher sample rates due to IR resampling artefacts. Measured values from the author: 44.1 kHz = -1 dB, 48 kHz = 0 dB (reference), 96 kHz = +3.5 dB, 192 kHz = +6.6 dB.
- **Expected:** Consistent output level across sample rates.
- **Workaround:** Provide separately recorded IRs at each target sample rate and load the correct one based on Engine.getSampleRate() (called from prepareToPlay, not onInit).
- **Status:** The author acknowledged the issue but noted a precise automatic correction is difficult because the relationship is non-linear across real-world IRs.

## Issue: Convolution offline render glitch at start of audio

- **Node:** filters.convolution
- **Type:** bug
- **Severity:** low
- **Source:** Forum tid:7006
- **Observed:** Convolution reverb produces a glitch artefact at the very beginning of an offline render when the render start point is positioned exactly at the start of audio. Suspected cause is a buffer not being cleared before the first block.
- **Expected:** Clean output from the first sample of an offline render.
- **Workaround:** Start the offline bounce a few milliseconds before the audio begins.

## Issue: Auto makeup gain on dynamics.comp is excessively hot

- **Node:** dynamics.comp (via Dynamics module)
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:1965
- **Observed:** The auto makeup gain calculation produces too much gain for typical compression settings. The stock formula scales aggressively with threshold and ratio.
- **Expected:** Makeup gain should approximate the average gain reduction to restore perceived loudness without overdriving.
- **Workaround:** Modify the makeup gain formula in hi_modules/effects/fx/Dynamics.cpp and recompile HISE from source.
- **Status:** Reported 2019. Check whether the formula was updated in later versions.

## Issue: Limiter knob ranges do not map correctly via processor ID connection

- **Node:** dynamics.limiter (standalone Limiter FX)
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:3499, tid:7143
- **Observed:** When connecting UI knobs to the limiter via processor ID, parameter ranges do not behave as expected. The value popup is meaningless and the mapping is incorrect.
- **Expected:** Correct parameter range mapping consistent with the node's internal ranges.
- **Workaround:** Use the limiter within the Dynamics module instead, which has properly configured ranges.
- **Status:** Reported by David Healey in 2021 and again in 2023. Check if fixed in recent versions.

## Issue: control.timer crashes DAW plugins and standalone apps

- **Node:** control.timer
- **Type:** bug
- **Severity:** high
- **Source:** Forum tid:14414, tid:13308
- **Observed:** The control.timer scriptnode crashes both DAW plugins (Ableton) and standalone apps.
- **Expected:** Stable operation in all deployment targets.
- **Workaround:** Replace with a tempo-synced ramp node and compare nodes to simulate timer engagement.
- **Status:** Active issue. No fix confirmed as of the reporting date.

## Issue: UI performance issues with flex_ahdsr envelope

- **Node:** envelope.flex_ahdsr
- **Type:** bug
- **Severity:** medium
- **Source:** Forum tid:14211
- **Observed:** UI performance problems (sluggishness, frame drops) when the flex_ahdsr envelope is active, reported by experienced user.
- **Expected:** Smooth UI performance with the envelope active.
- **Workaround:** None known. The author requested a minimal reproducer.
- **Status:** Acknowledged by the author (January 2026). Check if fixed in later builds.
