<!-- Diagram triage:
  - No diagrams specified in Phase 1 data (empty diagrams array)
-->

# Sampler

Sampler provides scripting control over a ModulatorSampler module - HISE's primary sample playback engine. When a script processor lives inside a Sampler module, the `Sampler` object is available as a global API class. For samplers elsewhere in the module tree, use `ChildSynth.asSampler()` to obtain a reference:

```js
const var mySampler = Synth.getChildSynth("Sampler1");
mySampler.asSampler().loadSampleMap("My SampleMap");
```

The class covers five main areas:

1. **Sample map loading** - from pool references, JSON arrays, base64 strings, or SFZ files.
2. **Round-robin group management** - single-group, multi-group, and per-event group control.
3. **Mic position purging** - enabling and disabling channels for memory management in multi-mic setups.
4. **Sample selection and property editing** - two APIs (legacy index-based and modern object-based via `Sample` objects).
5. **Timestretching** - four modes from disabled to tempo-synced, configured via a JSON options object.

The class uses `Sampler.*` constants to identify sample properties when reading or writing values. These are used with the legacy selection API (`getSoundProperty` / `setSoundProperty`) and with `Sample` object bracket syntax:

| Constant | Value | Description |
|----------|-------|-------------|
| `Sampler.FileName` | 1 | Audio file path |
| `Sampler.Root` | 2 | Root note |
| `Sampler.HiKey` / `Sampler.LoKey` | 3 / 4 | Key range |
| `Sampler.HiVel` / `Sampler.LoVel` | 6 / 5 | Velocity range |
| `Sampler.RRGroup` | 7 | Round-robin group index |
| `Sampler.Volume` | 8 | Gain in decibels |
| `Sampler.Pan` | 9 | Stereo panning |
| `Sampler.Normalized` | 10 | Enable sample normalisation |
| `Sampler.Pitch` | 11 | Pitch factor in cents |
| `Sampler.SampleStart` / `Sampler.SampleEnd` | 12 / 13 | Sample offsets |
| `Sampler.SampleStartMod` | 14 | Sample start modulation range |
| `Sampler.LoopStart` / `Sampler.LoopEnd` | 15 / 16 | Loop boundaries |
| `Sampler.LoopXFade` | 17 | Loop crossfade length |
| `Sampler.LoopEnabled` | 18 | Enable sample looping |
| `Sampler.ReleaseStart` | 19 | Release trigger offset in samples |
| `Sampler.LowerVelocityXFade` / `Sampler.UpperVelocityXFade` | 20 / 21 | Velocity crossfade lengths |
| `Sampler.SampleState` | 22 | Normal / Disabled / Purged |
| `Sampler.Reversed` | 23 | Play sample in reverse |
| `Sampler.NumQuarters` | 24 | Length in quarter notes (tempo-synced stretching) |

> [!Tip:Sample operations execute asynchronously] Most functions that change samples (loading maps, clearing, purging) execute asynchronously - they kill active voices and schedule the operation. Use `ScriptPanel.setLoadingCallback()` to keep your UI updated during these operations.
>
> [!Tip:Disable round robin before group management] All group management methods (`setActiveGroup`, `setMultiGroupIndex`, and their per-event variants) require `enableRoundRobin(false)` to be called first.

## Common Mistakes

- **Disable RR before group management**
  **Wrong:** `Sampler.setActiveGroup(1);` without disabling RR
  **Right:** `Sampler.enableRoundRobin(false); Sampler.setActiveGroup(1);`
  *Group management methods require round-robin to be disabled first, otherwise a script error is thrown.*

- **Defer loadSampleMap with a timer**
  **Wrong:** Calling `loadSampleMap()` directly from a ComboBox callback
  **Right:** Using a timer to defer the load (e.g. 50ms poll interval)
  *Direct loading from UI callbacks can cause audio glitches. A timer decouples the UI event from the asynchronous loading operation.*

- **Call refreshRRMap before querying groups**
  **Wrong:** `Sampler.getRRGroupsForMessage(60, 100);` without calling `refreshRRMap()` first
  **Right:** `Sampler.refreshRRMap();` in onInit, then `Sampler.getRRGroupsForMessage(60, 100);` in callbacks
  *The RR map must be rebuilt at compile time before querying group counts for note/velocity combinations.*

- **Use getSampleMapAsBase64 for persistence**
  **Wrong:** Saving the JSON array from `loadSampleMapFromJSON()` for preset persistence
  **Right:** Using `Sampler.getSampleMapAsBase64()` to capture the complete state
  *The base64 format includes post-load edits (e.g. sample range changes made via an AudioWaveform). Re-saving only the original JSON array loses those modifications.*
