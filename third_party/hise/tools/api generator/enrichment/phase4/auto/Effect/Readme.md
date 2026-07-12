<!-- Diagram triage:
  - (no diagrams specified in Phase 1 data)
-->

# Effect

Effect is a script handle for controlling any audio effect module in the HISE module tree. It wraps filters, reverbs, delays, dynamics processors, and any other effect type behind a uniform interface for parameter control, bypass, state serialisation, and metering.

Obtain a reference with `Synth.getEffect()` during `onInit` and store it as a `const var`:

```js
const var fx = Synth.getEffect("MyFilter");
```

Each Effect instance exposes the wrapped module's parameters as named constants (e.g. `fx.Frequency`, `fx.Gain`, `fx.Q`). The available constants depend on which effect type the handle wraps. Use these instead of raw integer indices for self-documenting, refactor-safe parameter access.

The class provides four main capability groups:

1. **Parameter access** - read and write effect parameters by index or name.
2. **Bypass and state** - enable/disable processing, and serialise/restore the full processor state as Base64 strings.
3. **Metering** - poll output levels for custom peak meter displays and query silence suspension status.
4. **Filter visualisation** - configure interactive draggable filter band displays for compatible effect types.

Effect shares its core API surface (parameter access, bypass, state serialisation, modulator chain management) with `Modulator`, `ChildSynth`, and `MidiProcessor`. Effect adds output level metering and draggable filter configuration.

> [!Tip:Get references in onInit, soft bypass for master effects] Effect references can only be obtained during `onInit` via `Synth.getEffect()`.
> Dynamically created effects use `Synth.addEffect()` or `Builder.create()`.
> Master effects use a soft bypass with fade-out to avoid clicks.

## Common Mistakes

- **Get effect references in onInit only**
  **Wrong:** `const var fx = Synth.getEffect("MyFX");` in `onNoteOn`
  **Right:** `const var fx = Synth.getEffect("MyFX");` in `onInit`
  *`Synth.getEffect()` can only be called during initialisation. Store the reference as a top-level const.*

- **Use named attribute constants**
  **Wrong:** `fx.setAttribute(0, 1000.0)` using raw integer indices
  **Right:** `fx.setAttribute(fx.Frequency, 1000.0)` using named constants
  *Named constants are generated per effect type. They make code self-documenting and survive parameter reordering.*

- **Use exportState for built-in effects**
  **Wrong:** `fx.exportScriptControls()` on a built-in effect
  **Right:** `fx.exportState()` on a built-in effect
  *`exportScriptControls()` and `restoreScriptControls()` only work on Script FX modules. Use the full state methods for built-in effects.*

- **Resync UI after restoreState**
  **Wrong:** Calling `restoreState()` and expecting connected UI components to update automatically
  **Right:** Calling `updateValueFromProcessorConnection()` on connected components after `restoreState()`
  *`restoreState()` does not send parameter change notifications. Connected knobs and sliders must be explicitly resynced.*
