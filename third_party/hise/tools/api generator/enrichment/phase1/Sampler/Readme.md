# Sampler -- Class Analysis

## Brief
Script handle for controlling ModulatorSampler: sample maps, round-robin groups, mic positions, timestretching.

## Purpose
The Sampler class provides scripting access to a ModulatorSampler module, HISE's primary sample playback engine. It covers sample map loading and saving (from files, JSON, base64, or SFZ), round-robin group management with per-event control, multi-mic position purging, sample selection and property editing, timestretching configuration, and release-start behavior. When a script processor lives inside a Sampler module, the `Sampler` object is automatically available as a global API class. For external samplers, use `ChildSynth.asSampler()`.

## Details

### Two Sample Selection Systems

The class contains two distinct ways to select and manipulate samples:

**Legacy selection** (index-based): Uses `selectSounds()` to fill an internal selection set, then `getSoundProperty()` / `setSoundProperty()` / `setSoundPropertyForSelection()` to read/write by integer index. The `getNumSelectedSounds()` and `loadSampleForAnalysis()` methods also operate on this internal set.

**Modern selection** (object-based): Uses `createSelection()`, `createSelectionFromIndexes()`, or `createSelectionWithFilter()` to return arrays of `Sample` objects. Each Sample object can then be manipulated directly through the Sample API. This is the preferred approach.

### Round-Robin Group Management

Group control requires disabling automatic round-robin first via `enableRoundRobin(false)`. After that:

- **Single group**: `setActiveGroup(index)` activates one group (one-based index)
- **Per-event group**: `setActiveGroupForEventId(eventId, index)` -- must be called from the audio thread (onNoteOn callback)
- **Multi-group**: `setMultiGroupIndex(groupIndex, enabled)` -- accepts a single index, an array of indices, or a MidiList object
- **Per-event multi-group**: `setMultiGroupIndexForEventId(eventId, groupIndex, enabled)` -- same flexibility
- **Group volume**: `setRRGroupVolume(groupIndex, dB)` -- use -1 for the currently active group. Only works with disabled crossfade tables.
- **RR map queries**: `getRRGroupsForMessage(note, vel)` returns available groups. Requires `refreshRRMap()` to be called first (at compile time).

### Timestretch Modes

Configured via `setTimestretchOptions()` with a JSON object supporting four modes (`"Disabled"`, `"VoiceStart"`, `"TimeVariant"`, `"TempoSynced"`) plus additional properties (`Tonality`, `SkipLatency`, `NumQuarters`, `PreferredEngine`). See `setTimestretchOptions()` for full mode descriptions and `getTimestretchOptions()` for the property schema.

### Release Start Options

Configured via `setReleaseStartOptions()` with a JSON object containing `ReleaseFadeTime`, `FadeGamma`, `UseAscendingZeroCrossing`, `GainMatchingMode`, and `PeakSmoothing`. Requires `HISE_SAMPLER_ALLOW_RELEASE_START` (enabled by default). See `setReleaseStartOptions()` and `getReleaseStartOptions()` for the full property schema and defaults.

### Sample Map Loading

Multiple loading paths exist, all using `killAllVoicesAndCall` for thread safety:

- `loadSampleMap(id)` -- loads by samplemap pool reference ID
- `loadSampleMapFromJSON(array)` -- loads from array of JSON objects (applies defaults for missing properties)
- `loadSampleMapFromBase64(string)` -- loads from zstd-compressed base64 string
- `loadSfzFile(file)` -- imports from SFZ format

Default properties applied when loading from JSON: LoVel=0, HiVel=127, LoKey=0, HiKey=127, Root=64, RRGroup=1.

### Threading Model

- **Audio-thread safe**: Group management methods (setActiveGroup, setMultiGroupIndex, etc.), attribute get/set, isNoteNumberMapped
- **Non-audio-thread**: Sample map loading, selection, property editing, mic position management, import -- all use `WARN_IF_AUDIO_THREAD`
- **Async patterns**: Property modifications use `callAsyncIfJobsPending`; map loading uses `killAllVoicesAndCall`
- **Per-event methods**: `setActiveGroupForEventId` and `setMultiGroupIndexForEventId` with eventId != -1 require the audio thread

## obtainedVia
`Sampler` (auto-registered when script processor is inside a Sampler module) or `ChildSynth.asSampler()`

## minimalObjectToken
sampler

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
| Pan | 9 | int | Stereo panning | SampleProperty |
| Normalized | 10 | int | Enable sample normalization | SampleProperty |
| Pitch | 11 | int | Pitch factor in cents (+/- 100) | SampleProperty |
| SampleStart | 12 | int | Start sample offset | SampleProperty |
| SampleEnd | 13 | int | End sample offset | SampleProperty |
| SampleStartMod | 14 | int | Sample start modulation range | SampleProperty |
| LoopStart | 15 | int | Loop start in samples | SampleProperty |
| LoopEnd | 16 | int | Loop end in samples | SampleProperty |
| LoopXFade | 17 | int | Loop crossfade length | SampleProperty |
| LoopEnabled | 18 | int | Enable sample looping | SampleProperty |
| ReleaseStart | 19 | int | Release trigger offset in samples | SampleProperty |
| LowerVelocityXFade | 20 | int | Lower velocity crossfade length | SampleProperty |
| UpperVelocityXFade | 21 | int | Upper velocity crossfade length | SampleProperty |
| SampleState | 22 | int | Sample state (Normal/Disabled/Purged) | SampleProperty |
| Reversed | 23 | int | Play sample in reverse | SampleProperty |
| NumQuarters | 24 | int | Length in quarter notes (for tempo-synced stretching) | SampleProperty |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Sampler.setActiveGroup(1);` without disabling RR | `Sampler.enableRoundRobin(false); Sampler.setActiveGroup(1);` | Group management methods require round-robin to be disabled first, otherwise a script error is thrown. |
| `Sampler.setActiveGroupForEventId(id, 2);` in onControl | `Sampler.setActiveGroupForEventId(id, 2);` in onNoteOn | Per-event group methods with eventId != -1 must be called from the audio thread (onNoteOn callback). |
| `Sampler.getRRGroupsForMessage(60, 100);` without refreshRRMap | `Sampler.refreshRRMap(); // in onInit` then `Sampler.getRRGroupsForMessage(60, 100);` | The RR map must be recalculated at compile time before querying group counts. |

## codeExample
```javascript
// The Sampler object is auto-available when the script is inside a Sampler module
const var sampler = Sampler;

// Load a sample map
sampler.loadSampleMap("MyInstrument");

// Create a selection of all samples and modify a property
const var allSamples = sampler.createSelectionFromIndexes(-1);
```

## Alternatives
- `Sample` -- handle to an individual sound within the sampler's current map (created by Sampler's selection methods)
- `AudioSampleProcessor` -- controls single audio file playback modules (AudioLoopPlayer), not multi-sample instruments

## Related Preprocessors
`USE_BACKEND` (createListFromGUISelection, setGUISelection), `HI_ENABLE_EXPANSION_EDITING` (importSamples), `HISE_SAMPLER_ALLOW_RELEASE_START` (release start options -- enabled by default).

## Diagnostic Ideas
Reviewed: Yes
Count: 2
- Sampler.setActiveGroup / setMultiGroupIndex / getRRGroupsForMessage -- timeline dependency (logged)
- Sampler.getRRGroupsForMessage -- timeline dependency (logged)
