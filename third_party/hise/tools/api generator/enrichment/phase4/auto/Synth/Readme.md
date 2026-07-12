<!-- Diagram triage:
  - synth-module-tree-search: CUT (topology of two search strategies is adequately explained by prose and the method-level docs already clarify which strategy each method uses; the fan-out would require listing 15+ method names to be accurate)
  - synth-midi-event-flow: CUT (the three note-on methods funnelling into one internal function is a simple linear pattern; the note-off path is already clear from the method docs and cross-references)
-->

# Synth

Synth is the global namespace for interacting with the parent sound generator that hosts your script processor. It is available automatically in every script callback - there is no need to create it. Its responsibilities fall into four areas:

1. **Module tree access** - retrieve handles to effects, modulators, samplers, MIDI processors, and child synths within the parent synth's subtree. A few methods (`getMidiPlayer`, `getRoutingMatrix`, `getWavetableController`, `getAllModulators`) search the entire module tree instead.
2. **MIDI event generation** - create artificial notes, controller events, pitch fades, and volume fades from script. All generated events are flagged as artificial.
3. **Timer system** - start and stop a periodic timer that fires the `onTimer` callback at a configurable interval.
4. **Keyboard and voice state** - query pressed keys, legato transitions, sustain pedal state, and active artificial events.

Store module references as top-level `const var` variables so they are cached across callbacks. Use `playNote` or `addNoteOn` to generate notes, and always store the returned event ID for later release via `noteOffByEventId`.

```js
// Typical onInit pattern
const var fx = Synth.getEffect("Delay1");
const var sampler = Synth.getSampler("MainSampler");
```

> [!Tip:Cache get methods in onInit variables] Most `get*()` methods can only be called in `onInit` because they allocate wrapper objects. Store the returned references in top-level variables and reuse them in callbacks. The module tree search is scoped to the parent synth's subtree unless noted otherwise on specific methods.

## Common Mistakes

- **Cache module references in onInit**
  **Wrong:** `var mod = Synth.getModulator("LFO1");` in `onNoteOn`
  **Right:** `const var mod = Synth.getModulator("LFO1");` in `onInit`
  *Most `get*()` methods are restricted to `onInit`. Calling them in MIDI callbacks causes errors or audio-thread allocations.*

- **Velocity zero may cause note-off**
  **Wrong:** `Synth.playNote(60, 0);`
  **Right:** `Synth.playNote(60, 1);`
  *`playNote` rejects velocity 0. Use `noteOffByEventId` to stop notes.*

- **Macro indices are 1-based**
  **Wrong:** `Synth.setMacroControl(0, 64.0);`
  **Right:** `Synth.setMacroControl(1, 64.0);`
  *The macro index is 1-based (1-8), not 0-based.*

- **Use playNoteFromUI for on-screen keyboards**
  **Wrong:** Using `Synth.playNote` for a custom on-screen keyboard
  **Right:** Using `Synth.playNoteFromUI` / `Synth.noteOffFromUI`
  *`playNoteFromUI` routes through the real MIDI input pipeline, so `isKeyDown` and `getNumPressedKeys` reflect the note. `playNote` creates artificial events that bypass keyboard state tracking.*

- **Disable retrigger kill for unison**
  **Wrong:** Forgetting `Synth.setShouldKillRetriggeredNote(false)` in a unison script
  **Right:** Call it in `onInit` before generating multiple voices per key
  *Without this, the synth kills existing voices when a new note arrives on the same pitch, defeating voice stacking.*
