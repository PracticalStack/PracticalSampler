<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# Sample

A Sample object is a handle to a single sound within a Sampler module's sample map. It provides direct read/write access to per-sample properties (key range, velocity range, volume, loop points, and more), supports loading sample audio into buffers for analysis, and allows programmatic restructuring of the sample map through duplication and deletion.

You obtain Sample objects from the parent Sampler's selection methods:

```js
const var sampler = Synth.getSampler("Sampler1");
const var selection = sampler.createSelection(".*");  // all samples
const var s = selection[0];                            // single Sample
```

Other selection methods include `Sampler.createSelectionFromIndexes()`, `Sampler.createSelectionWithFilter()`, `Sampler.createListFromGUISelection()`, and `Sampler.createListFromScriptSelection()`. You can also obtain a Sample from `Sample.duplicateSample()`.

Sample properties are accessed using integer constants defined on the `Sample` class. Three equivalent access styles are supported:

```js
var root = s.get(Sample.Root);       // getter method
var root = s[Sample.Root];           // bracket with constant
var root = s["Root"];                // bracket with string identifier
```

The full set of property constants:

| Constant | Value | Description |
|----------|-------|-------------|
| `Sample.FileName` | 1 | Audio file path (returns a String; all others return integers) |
| `Sample.Root` | 2 | Root note |
| `Sample.HiKey` | 3 | Highest mapped key |
| `Sample.LoKey` | 4 | Lowest mapped key |
| `Sample.LoVel` | 5 | Lowest mapped velocity |
| `Sample.HiVel` | 6 | Highest mapped velocity |
| `Sample.RRGroup` | 7 | Round-robin group index |
| `Sample.Volume` | 8 | Gain in decibels (integer, not float) |
| `Sample.Pan` | 9 | Stereo balance (-100 = left, 100 = right) |
| `Sample.Normalized` | 10 | Auto-normalise to 0 dB (0/1) |
| `Sample.Pitch` | 11 | Fine-tuning in cents (+/- 100) |
| `Sample.SampleStart` | 12 | Start offset in samples |
| `Sample.SampleEnd` | 13 | End offset in samples |
| `Sample.SampleStartMod` | 14 | Sample start modulation range |
| `Sample.LoopStart` | 15 | Loop start in samples |
| `Sample.LoopEnd` | 16 | Loop end in samples |
| `Sample.LoopXFade` | 17 | Loop crossfade length |
| `Sample.LoopEnabled` | 18 | Enable looping (0/1) |
| `Sample.ReleaseStart` | 19 | Release start offset (0 = disabled) |
| `Sample.LowerVelocityXFade` | 20 | Lower velocity crossfade length |
| `Sample.UpperVelocityXFade` | 21 | Upper velocity crossfade length |
| `Sample.SampleState` | 22 | Sample state (see below) |
| `Sample.Reversed` | 23 | Play in reverse (0/1) |

The `SampleState` property accepts three values:

| Value | State | Description |
|-------|-------|-------------|
| 0 | Normal | Sample is loaded and playable |
| 1 | Disabled | Sample is excluded from playback |
| 2 | Purged | Sample data is unloaded from memory |

> [!Tip:Property ranges auto-clipped and interdependent] Property values are automatically clipped to valid ranges on `set()`. Some
> property ranges are interdependent - for example, `LoopEnd` cannot exceed
> `SampleEnd`. Use `getRange()` to query the current valid bounds before
> setting loop-related properties.
>
> [!Tip:Use Script Watch Table to inspect samples] `trace()` does not work on Sample objects. Use the Script Watch Table to
> inspect their contents.
>
> [!Tip:Constants also accessible via Sampler class] These property constants are also accessible via the `Sampler` class
> (e.g. `Sampler.SampleEnd`) for backwards compatibility.

## Common Mistakes

- **replaceAudioFile not for monolithic samples**
  **Wrong:** `s.replaceAudioFile(bufferArray)` on monolithic samples
  **Right:** Use `replaceAudioFile()` only with non-monolithic sample files.
  *Monolithic samples (.ch1 files) cannot be overwritten - the method throws a script error.*

- **Discard reference after deleteSample**
  **Wrong:** Using a Sample reference after calling `deleteSample()`
  **Right:** Discard the reference immediately after deletion.
  *The underlying sound is removed from the sample map. Any further method call on the deleted reference throws "Sound does not exist".*

- **Set velocity ranges twice for both bounds**
  **Wrong:** Setting `LoVel` once and assuming the value sticks
  **Right:** Set both `LoVel` and `HiVel` together, potentially twice in succession.
  *Auto-clipping is interdependent: setting `LoVel` higher than the current `HiVel` silently clamps it. When widening a range, set `HiVel` first; when narrowing, set `LoVel` first.*

- **Use custom properties not parallel arrays**
  **Wrong:** Storing analysis results in a parallel array alongside a sample selection
  **Right:** Attach computed data to `getCustomProperties()` on each Sample.
  *Parallel arrays break when the selection is sorted or filtered. Custom properties travel with the Sample object through any reordering.*

- **Check getRange before setting loop points**
  **Wrong:** Setting loop points without checking `getRange()` first
  **Right:** Query `getRange(Sample.LoopEnd)` to get the current valid bounds before setting.
  *Loop property ranges are dynamic - `LoopEnd`'s maximum depends on `SampleEnd`, and `LoopStart`'s minimum depends on `SampleStart` plus `LoopXFade`. Values outside the valid range are silently clamped.*
