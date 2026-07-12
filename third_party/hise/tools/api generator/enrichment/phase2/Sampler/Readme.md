# Sampler -- Project Context

## Project Context

### Real-World Use Cases
- **Multi-mic concert instrument**: A piano or orchestral sampler with 3+ mic positions (Close/Mid/Far) uses multiple `Synth.getSampler()` references in an array, with `purgeMicPosition()` driven by a mode matrix for memory management. The mic purge state is coordinated with per-mic gain effects for level control.
- **Multi-articulation sampler with group switching**: An instrument sampler disables automatic round-robin via `enableRoundRobin(false)` and uses `setActiveGroup()` in `onNoteOn`/`onNoteOff` callbacks to select groups based on performance context (note duration, velocity, keyswitch state). This pattern is common for release samples that need different groups for short vs. sustained notes.
- **User-importable sample player**: A plugin that lets users drag-drop or browse for audio files uses the `parseSampleFile()` -> `loadSampleMapFromJSON()` pipeline for import, then `getSampleMapAsBase64()` / `loadSampleMapFromBase64()` for preset persistence. The base64 format stores the complete sample map state in a single string that fits into a user preset panel value.
- **Multi-sampler instrument with category browser**: A synthesizer or drum machine with multiple sampler oscillators uses `getSampleMapList()` to build a browsable sound library, parsing the list by folder separator to extract categories and sound names, then loads selections with `loadSampleMap()`.
- **Dual-layer sampler with deferred loading**: A plugin with two independent sampler layers uses timer-based deferred loading to avoid audio thread hitches when switching sounds. A `TimerObject` polls for changes at 50ms intervals and calls `loadSampleMap()` only when a change is detected.

### Complexity Tiers
1. **Basic sampler** (most common): `loadSampleMap()`, `getSampleMapList()`, `getCurrentSampleMapId()`. Sufficient for a single-sampler instrument with preset-driven map selection.
2. **Multi-mic sampler**: Adds `purgeMicPosition()`, `getNumMicPositions()`, `getMicPositionName()`, `isMicPositionPurged()`. Requires understanding the mic suffix naming convention and coordinating purge state with gain effects.
3. **Manual round-robin control**: Adds `enableRoundRobin(false)`, `setActiveGroup()`, `refreshRRMap()`, `getRRGroupsForMessage()`. Used when the automatic RR cycling is insufficient and the script needs to select groups based on musical context.
4. **User sample import**: Adds `parseSampleFile()`, `loadSampleMapFromJSON()`, `getSampleMapAsBase64()`, `loadSampleMapFromBase64()`, `clearSampleMap()`, `createSelection()`. Full workflow for user-importable content with preset persistence.
5. **Advanced sample manipulation**: Adds `createSelectionWithFilter()`, `Sample.set()`, `refreshInterface()`. Used for IDE tooling scripts that programmatically modify sample properties like velocity mapping.

### Practical Defaults
- Use `Synth.getSampler("SamplerName")` to obtain a Sampler reference when the script is not inside the sampler module. Cache the reference in a `const var` at init time.
- Use `enableRoundRobin(false)` in `onInit` before any manual group selection. Forgetting this is the most common Sampler API mistake.
- Use timer-deferred loading (50ms poll interval) when `loadSampleMap()` is triggered by UI controls. Direct calls from control callbacks work but can cause audio glitches during preloading.
- Use `createSelection(".*")` or `createSelectionFromIndexes(-1)` to select all samples. The modern Sample-object API is preferred over the legacy `selectSounds()` workflow.
- Call `refreshInterface()` after programmatically modifying sample properties via `Sample.set()` to update the HISE sample editor and any connected UI displays.

### Integration Patterns
- `Synth.getSampler()` -> `Sampler.purgeMicPosition()` + `Synth.getEffect()` -> `Effect.setAttribute()` - Multi-mic management coordinates sampler purge state with per-mic gain effects for a complete mic routing system.
- `Sampler.getSampleMapList()` -> `String.split("/")` -> `Sampler.loadSampleMap()` - Sample map IDs use folder paths as category separators, enabling a two-column browser pattern (categories + sounds).
- `Sampler.enableRoundRobin(false)` -> `Sampler.setActiveGroup()` in `onNoteOn`/`onNoteOff` - Manual group selection based on musical context (note duration, velocity, articulation state).
- `Sampler.parseSampleFile()` -> `Sampler.loadSampleMapFromJSON()` -> `Sampler.getSampleMapAsBase64()` -> panel `saveInPreset` - Complete user sample import and preset persistence chain.
- `Sampler.isNoteNumberMapped()` -> `Engine.setKeyColour()` - Keyboard visualization that highlights playable keys based on the loaded sample map's note coverage.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `loadSampleMap()` directly from a ComboBox callback | Using a timer to defer the load (50ms poll) | Direct loading from UI callbacks can cause audio glitches. A timer decouples the UI event from the loading operation. |
| Using inline `Content.getComponent("Sampler")` calls in callbacks | Caching `const var sampler = Synth.getSampler("Name")` at init | Repeated lookups waste CPU. Cache all sampler references as `const var` in `onInit`. |
| Iterating samplers with index-based for loops for purge operations | Using `for (s in samplers) s.purgeMicPosition(...)` | `for...in` is faster and cleaner when the index is not needed. |
