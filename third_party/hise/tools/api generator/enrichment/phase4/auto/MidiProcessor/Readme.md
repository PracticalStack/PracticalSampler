<!-- Diagram triage:
  - No diagram specifications in Phase 1 data.
-->

# MidiProcessor

MidiProcessor is a script handle for controlling any MIDI processor module in the signal chain. Obtain a reference with `Synth.getMidiProcessor()` in `onInit` and use it to read and write parameters, toggle bypass, and serialise the module's state.

```js
const var mp = Synth.getMidiProcessor("Arpeggiator1");
```

Each MidiProcessor handle exposes dynamic constants that map parameter names to their integer indices, so you can write `mp.setAttribute(mp.Intensity, 0.5)` instead of using raw index numbers. Bracket assignment syntax also works for writing: `mp["Intensity"] = 0.5`.

The class provides two levels of state serialisation:

1. **Full state** - `exportState()` / `restoreState()` capture and restore all parameters and internal state. Works on any MIDI processor type.
2. **Script controls only** - `exportScriptControls()` / `restoreScriptControls()` capture and restore only UI control values (knobs, sliders, buttons) without triggering recompilation. These only work on script processors - calling them on built-in modules like Transposer or Arpeggiator throws an error.

For modules that are MidiPlayer instances, use `asMidiPlayer()` to cast the handle and access MIDI file playback methods.

> [!Tip:Cache references in onInit] MidiProcessor references must be cached in `onInit`. `Synth.getMidiProcessor()` cannot be called from callbacks. If you need multiple references (e.g. an array of MidiMuter modules), build the array in a loop at init time and index into it at runtime.

## Common Mistakes

- **Use getAttribute not bracket read**
  **Wrong:** `var v = mp["Intensity"];`
  **Right:** `var v = mp.getAttribute(mp.Intensity);`
  *Bracket read always returns 1.0 due to an internal limitation. Use `getAttribute()` for reading parameter values.*

- **Use exportState for built-in modules**
  **Wrong:** `mp.exportScriptControls()` on a Transposer
  **Right:** `mp.exportState()` on a Transposer
  *`exportScriptControls()` only works on script processors. Use `exportState()` for built-in MIDI modules.*

- **Cache processor references in onInit**
  **Wrong:** `Synth.getMidiProcessor("MidiMuter" + i)` inside a callback
  **Right:** Cache all references in `onInit`: `for (i = 0; i < N; i++) muters.push(Synth.getMidiProcessor("MidiMuter" + (i + 1)));`
  *`Synth.getMidiProcessor()` is restricted to `onInit`. Cache references at init time and index into the array at runtime.*

- **Use named constants not raw indices**
  **Wrong:** `mp.setAttribute(0, value)`
  **Right:** `mp.setAttribute(mp.SomeParameter, value)`
  *Raw indices are fragile and unreadable. Dynamic constants are module-specific and self-documenting.*
