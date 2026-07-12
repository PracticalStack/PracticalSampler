<!-- Diagram triage:
  - No diagrams specified in Phase 1 data for Engine class.
-->

# Engine

Engine is the central namespace in HISEScript, providing the global `Engine` object that is available in every script processor without creation. It serves as the primary entry point for four broad areas of functionality:

1. **Object factories** - creating timers, broadcasters, preset handlers, DSP networks, MIDI lists, modulation matrices, and other scripting API objects via the `create*` methods.
2. **Unit conversion** - converting between samples, milliseconds, quarter beats, decibels, gain factors, MIDI note numbers, frequencies, semitones, and pitch ratios.
3. **Global state management** - controlling BPM, global pitch, user presets, expansions, keyboard colours, macro names, and audio engine settings.
4. **System queries and resource pools** - querying the OS, sample rate, buffer size, CPU usage, device info, and loading fonts, images, and audio files into resource pools.

The typical initialisation sequence in `onInit` is: load fonts, load resource pools, configure macro names and preset tags, then create factory objects (timers, broadcasters, preset handlers) for the plugin's runtime behaviour.

> [!Tip:IDE-only methods become no-ops in exports] Several methods are IDE-only and become no-ops in exported plugins: `clearMidiFilePool`, `clearSampleMapPool`, `rebuildCachedPools`, `loadImageIntoPool`, and `getSampleFilesFromDirectory`. These are safe to leave in production code but will have no effect. The deprecated methods `getZoomLevel`, `setZoomLevel`, `setDiskMode`, and `loadFont` have been replaced by the `Settings` class and `loadFontAs` respectively.

## Common Mistakes

- **Cannot load presets during onInit**
  **Wrong:** `Engine.loadUserPreset("MyPreset")` in `onInit`
  **Right:** Call `loadUserPreset` from a button callback or timer.
  *Loading user presets during initialisation is explicitly rejected with an error message.*

- **Use getHostBpm or TransportHandler**
  **Wrong:** `Engine.getPlayHead().bpm`
  **Right:** `Engine.getHostBpm()` or use `Engine.createTransportHandler()`.
  *The play head object's properties are not populated. Use the dedicated BPM methods or a TransportHandler for reliable host transport data.*

- **Macro indices are 1-based**
  **Wrong:** `Engine.getMacroName(0)`
  **Right:** `Engine.getMacroName(1)`
  *Macro indices are 1-based (1-8), not 0-based.*

- **Set all 128 key colours**
  **Wrong:** Calling `Engine.setKeyColour()` only for mapped notes.
  **Right:** Set all 128 keys, using a dim colour for unmapped notes.
  *Skipping unmapped keys leaves stale colours from a previous instrument or preset state.*

- **Filter module state with RemovedProperties**
  **Wrong:** `Engine.addModuleStateToUserPreset("MyEQ")` with just a string.
  **Right:** Pass a JSON object with `RemovedProperties` to exclude noise like routing matrix and bypass state.
  *Without filtering, the full module state is saved, bloating presets and causing unexpected state restoration.*
