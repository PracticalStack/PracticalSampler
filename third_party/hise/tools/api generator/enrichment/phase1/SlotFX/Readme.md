# SlotFX -- Class Analysis

## Brief
Dynamic effect slot handle for runtime loading, swapping, and querying of effect modules.

## Purpose
The `SlotFX` object is a script-level handle to a dynamic effect slot in the HISE module tree. Unlike the static `Effect` handle (which wraps a fixed module), SlotFX manages a container that can load and swap different effect types at runtime. It wraps either a `HotswappableProcessor` (classic SlotFX module or HardcodedMasterFX) or a `DspNetwork::Holder` (for scriptnode network loading). The handle provides methods to load effects by type name, query available modules, retrieve parameter properties, swap effects between slots, and clear the slot back to a unity-gain passthrough.

## Details

### Dual-Mode Architecture

SlotFX operates in two modes depending on the underlying processor type, detected via `dynamic_cast`:

| Mode | Underlying Type | setEffect returns | Use Case |
|------|----------------|-------------------|----------|
| Classic | `HotswappableProcessor` (SlotFX module, HardcodedMasterFX) | `Effect` handle | Loading built-in effect types |
| ScriptNode | `DspNetwork::Holder` (Script FX modules) | `DspNetwork` object | Loading compiled scriptnode networks |

Each API method tries the HotswappableProcessor path first, then falls back to DspNetwork::Holder.

### Classic Mode (HotswappableProcessor)

The SlotFX C++ module maintains an internal list of allowed effect types, built at construction from `EffectProcessorChainFactoryType` with a constrainer. The following types are excluded:

- Polyphonic effects (PolyFilterEffect, PolyshapeFX)
- Harmonic filters (HarmonicFilter, HarmonicMonophonicFilter)
- Routing effects (StereoEffect, RouteEffect)
- Nested SlotFX

See `setEffect()` for the full effect loading workflow and `clear()` for the passthrough reset behavior. An internal `isClear` flag enables a fast-path that skips all processing when EmptyFX is loaded.

### ScriptNode Mode (DspNetwork::Holder)

In this mode, `setEffect()` loads a scriptnode network by name. See `getModuleList()` for `USE_BACKEND` limitations when listing available networks.

### Effect Swapping

See `swap()` for the atomic exchange mechanism between two classic-mode SlotFX instances.

### Soft Bypass

The `setBypassed` method is declared but not registered -- it cannot be called from script. Use the returned `Effect` handle's `setBypassed()` instead.

## obtainedVia
`Synth.getSlotFX("SlotId")` (onInit only), or via `Builder.create()` to add the module then `Synth.getSlotFX()` to obtain the handle.

## minimalObjectToken
slot

## Constants
None. SlotFX has no hardcoded constants. The underlying processor typically has no parameters of its own (SlotFX module has zero parameters).

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| ScriptParameters | Object | Maps scripted UI component names to indices (only present when wrapping a Script FX module). |
| (per-parameter) | int | If the underlying processor has parameters (e.g. HardcodedMasterFX), each parameter name is registered as a constant mapping to its attribute index. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `slot.setBypassed(true)` | `slot.getCurrentEffect().setBypassed(true)` | `setBypassed` is declared but not registered on SlotFX. Use the Effect handle returned by `getCurrentEffect()` or `setEffect()` instead. |
| `slot.setEffect("PolyFilter")` | `slot.setEffect("SimpleFilter")` | Polyphonic effects are excluded by the SlotFX constrainer. Only monophonic/master effect types can be loaded. |

## codeExample
```javascript
// Get a reference to a SlotFX module in onInit
const var slot = Synth.getSlotFX("MyEffectSlot");

// Load a reverb into the slot
const var fx = slot.setEffect("SimpleReverb");

// Control the loaded effect
fx.setAttribute(fx.Size, 0.8);
```

## Alternatives
- `Effect` -- static handle to a fixed effect already in the signal chain, while SlotFX dynamically loads/swaps effect types at runtime.

## Related Preprocessors
`USE_BACKEND` -- affects `getModuleList()` in DspNetwork::Holder mode (network file listing only available in HISE IDE).

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: SlotFX methods have clear error reporting (reportScriptError for invalid slots/names) and no silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
