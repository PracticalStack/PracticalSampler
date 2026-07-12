<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# Modulator

Modulator is a script handle for controlling any modulation source in the HISE module tree - LFOs, envelopes, velocity modulators, constant modulators, and modulator chains. It provides control over:

1. Parameter access - read and write module attributes by index or name, including bracket-operator syntax (`mod["Frequency"] = 2.5`).
2. Modulation intensity - set the depth of modulation with mode-dependent ranges.
3. Bypass state - toggle modulators on and off from UI callbacks.
4. Global modulation routing - create dynamic or static connections to sources in a GlobalModulatorContainer.
5. State serialisation - capture and restore complete module configurations as base64 strings.

The intensity range depends on the modulation mode set by the parent chain:

| Mode | Intensity Range | Behaviour |
|------|----------------|-----------|
| GainMode | 0.0 - 1.0 | Multiplied with signal |
| PitchMode | -12.0 - 12.0 (semitones) | Added to pitch buffer |
| PanMode | -1.0 - 1.0 | Panning offset |
| GlobalMode | -1.0 - 1.0 | Intensity ignored in processing |
| OffsetMode | -1.0 - 1.0 | Added per-modulator with intensity |
| CombinedMode | 0.0 - 1.0 | Per-modulator GainMode or OffsetMode |

Each Modulator exposes its parameters as dynamic constants (e.g., `mod.Frequency`, `mod.Attack`) that map to attribute indices, so you can use named constants instead of raw integers with `setAttribute` and `getAttribute`.

```js
const var mod = Synth.getModulator("LFO1");
```

> [!Tip:Get references in onInit only] `Synth.getModulator()` must be called in `onInit`. Store the reference as a `const var` at the top level - it cannot be called from other callbacks.

## Common Mistakes

- **Cache modulator references in onInit**
  **Wrong:** `var mod = Synth.getModulator("LFO1");` in `onNoteOn`
  **Right:** `const var mod = Synth.getModulator("LFO1");` in `onInit`
  *`getModulator` is restricted to `onInit`. Store references as top-level const variables.*

- **Pitch intensity is in semitones**
  **Wrong:** `mod.setIntensity(1.0)` on a pitch modulator expecting full range
  **Right:** `mod.setIntensity(12.0)` for a full octave, or the desired semitone value
  *PitchMode intensity is in semitones (-12 to 12), not a normalised 0-1 range. A value of 1.0 gives only 1 semitone of bend.*

- **Track connections to prevent duplicates**
  **Wrong:** Calling `addGlobalModulator` without checking if a connection already exists
  **Right:** Track active connections and check before creating new ones
  *Duplicate connections stack - each call adds another modulator to the chain, doubling the modulation depth.*

- **Use static variant for voice-start sources**
  **Wrong:** Using `addGlobalModulator` for velocity or note-number sources
  **Right:** Use `addStaticGlobalModulator` for voice-start sources
  *Voice-start modulators produce a single value at note-on that never changes mid-note. The static variant avoids wasting CPU on per-block polling.*

- **Poll getCurrentLevel in timer callback**
  **Wrong:** Calling `getCurrentLevel` in `onControl` or `onNoteOn`
  **Right:** Poll in a timer callback at ~30ms intervals
  *The display value updates once per audio buffer. Polling from non-periodic callbacks gives stale or inconsistent readings.*
