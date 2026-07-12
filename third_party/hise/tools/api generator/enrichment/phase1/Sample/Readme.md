# Sample -- Class Analysis

## Brief
Handle to a single sampler sound for reading/writing per-sample properties, audio data, and duplication.

## Purpose
The Sample class is a scripting wrapper around a ModulatorSamplerSound within a Sampler module's current sample map. It provides direct access to per-sample properties (key range, velocity range, loop points, volume, pan, etc.) via integer constants or string identifiers, supports bracket-based property access through the AssignableObject interface, and allows loading sample audio into buffers for analysis or replacing audio file content. Sample objects are created by the Sampler class's selection methods (createSelection, createSelectionFromIndexes, createSelectionWithFilter) and represent the modern, object-based approach to sample manipulation.

## Details

### Property Access Patterns

Sample implements AssignableObject, enabling three equivalent access styles:

```javascript
// Integer constant index
var root = sample.get(Sample.Root);
sample.set(Sample.Root, 60);

// Bracket with constant
var root = sample[Sample.Root];
sample[Sample.Root] = 60;

// Bracket with string identifier
var root = sample["Root"];
sample["Root"] = 60;
```

String identifiers are resolved at parse time via `getCachedIndex()` for runtime performance. The string names match the SampleIds identifiers exactly (e.g., "Root", "HiKey", "LoVel").

### Return Type Behavior

See `get()` for details. All non-FileName properties return integers, including Volume (dB) and Pan.

### Value Clipping and Dynamic Ranges

Values are automatically clipped to valid ranges on `set()`. Ranges are interdependent -- see `getRange()` for querying current valid bounds. Setting certain properties triggers cascading adjustments to dependent properties (e.g., SampleStart changes may adjust LoopXFade, LoopStart, SampleStartMod). See `set()` for the full list.

### Custom Properties

See `getCustomProperties()`. This data is transient and NOT persisted in the sample map.

### Multi-Mic Audio Loading

See `loadIntoBufferArray()`. Returns a flat array of channel buffers across all mic positions.

## obtainedVia
`Sampler.createSelection(regex)`, `Sampler.createSelectionFromIndexes(index)`, `Sampler.createSelectionWithFilter(function)`, or `Sample.duplicateSample()`

## minimalObjectToken
s

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| FileName | 1 | int | Audio file path | SampleProperty |
| Root | 2 | int | Root note | SampleProperty |
| HiKey | 3 | int | Highest mapped key | SampleProperty |
| LoKey | 4 | int | Lowest mapped key | SampleProperty |
| LoVel | 5 | int | Lowest mapped velocity | SampleProperty |
| HiVel | 6 | int | Highest mapped velocity | SampleProperty |
| RRGroup | 7 | int | Round-robin group index | SampleProperty |
| Volume | 8 | int | Gain in decibels | SampleProperty |
| Pan | 9 | int | Stereo panning (-100 to 100) | SampleProperty |
| Normalized | 10 | int | Enable sample normalization (0/1) | SampleProperty |
| Pitch | 11 | int | Pitch factor in cents (+/- 100) | SampleProperty |
| SampleStart | 12 | int | Start sample offset | SampleProperty |
| SampleEnd | 13 | int | End sample offset | SampleProperty |
| SampleStartMod | 14 | int | Sample start modulation range | SampleProperty |
| LoopStart | 15 | int | Loop start in samples | SampleProperty |
| LoopEnd | 16 | int | Loop end in samples | SampleProperty |
| LoopXFade | 17 | int | Loop crossfade length | SampleProperty |
| LoopEnabled | 18 | int | Enable sample looping (0/1) | SampleProperty |
| ReleaseStart | 19 | int | Release trigger offset in samples | SampleProperty |
| LowerVelocityXFade | 20 | int | Lower velocity crossfade length | SampleProperty |
| UpperVelocityXFade | 21 | int | Upper velocity crossfade length | SampleProperty |
| SampleState | 22 | int | Sample state (0=Normal, 1=Disabled, 2=Purged) | SampleProperty |
| Reversed | 23 | int | Play sample in reverse (0/1) | SampleProperty |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `s.replaceAudioFile(bufferArray);` on monolith samples | Use only with non-monolithic sample files | replaceAudioFile throws a script error when called on monolithic samples -- audio data in .ch1 files cannot be overwritten. |
| Using a Sample reference after `deleteSample()` | Discard the reference after deletion | After deleteSample(), the underlying sound is removed; any further method call on the same Sample object throws "Sound does not exist". |

## codeExample
```javascript
// Get all samples from the sampler
const var allSamples = Sampler.createSelectionFromIndexes(-1);

// Access a single sample
const var s = allSamples[0];

// Read and modify properties
var root = s.get(Sample.Root);
s.set(Sample.Root, 60);

// Bracket syntax also works
s[Sample.Volume] = -6;
```

## Alternatives
- `Sampler` -- module-level handle that manages the entire sample map and all its sounds; use its legacy selectSounds/getSoundProperty/setSoundProperty for index-based batch operations, or its createSelection methods to obtain Sample objects.

## Related Preprocessors
`HI_ENABLE_EXPANSION_EDITING` (deleteSample -- method body is empty without this flag).

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Sample is a simple property-access handle with no timeline dependencies, no silent-failure preconditions, and automatic value clipping. All error conditions produce immediate script errors.
