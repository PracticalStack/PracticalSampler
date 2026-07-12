<!-- Diagram triage:
  - (none) No diagram specs in Phase 1 data
-->

# SlotFX

SlotFX is a dynamic effect slot that can load, swap, and clear effect modules at runtime. Unlike a static `Effect` handle that wraps a fixed module in the signal chain, SlotFX manages a container whose contents can change during playback.

The slot operates in one of two modes, depending on the underlying module type:

| Mode | Module type | `setEffect()` returns | Use case |
|------|------------|----------------------|----------|
| Classic | SlotFX, HardcodedMasterFX | `Effect` handle | Loading built-in effect types |
| ScriptNode | Script FX (DspNetwork) | `DspNetwork` object | Loading compiled scriptnode networks |

In classic mode, the slot maintains a filtered list of allowed effect types - polyphonic effects, routing effects, harmonic filters, and nested SlotFX modules are excluded. Use `getModuleList()` to query which types are available, then `setEffect()` to load one. The returned handle gives you direct control over the loaded effect's parameters and bypass state.

```js
const var slot = Synth.getSlotFX("MyEffectSlot");
```

> [!Tip:Bypass via loaded Effect handle instead] `setBypassed()` is not available on SlotFX. To bypass the loaded effect, call `setBypassed()` on the `Effect` handle returned by `setEffect()` or `getCurrentEffect()`.

## Common Mistakes

- **Use getCurrentEffect to bypass loaded effect**
  **Wrong:** `slot.setBypassed(true)`
  **Right:** `slot.getCurrentEffect().setBypassed(true)`
  *`setBypassed` is declared but not registered on SlotFX. Use the Effect handle returned by `getCurrentEffect()` or `setEffect()` instead.*

- **Use full C++ module type name**
  **Wrong:** `slot.setEffect("PolyFilter")`
  **Right:** `slot.setEffect("SimpleFilter")`
  *Polyphonic effects are excluded by the SlotFX constrainer. Only monophonic and master effect types can be loaded.*

- **Get loaded effect via getCurrentEffect**
  **Wrong:** `Synth.getEffect("MySlot_SimpleReverb")` after loading
  **Right:** `const var fx = slot.setEffect("SimpleReverb");`
  *`setEffect()` returns the Effect handle directly. Looking up the child by its internal name is fragile and depends on an undocumented naming convention.*

- **Share knob update logic across effects**
  **Wrong:** Separate switch/case per effect with duplicated knob binding code
  **Right:** A data-driven configuration object with a generic `setEffect()` and parameter binding loop
  *A config object mapping effect names to module IDs, parameter arrays, and ranges eliminates per-effect code duplication and scales to any number of effects.*
