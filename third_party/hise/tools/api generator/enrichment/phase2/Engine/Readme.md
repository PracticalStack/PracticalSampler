# Engine -- Project Context

## Project Context

### Real-World Use Cases
- **Plugin initialization hub**: Every plugin uses Engine as the first-contact API during `onInit` -- loading fonts, configuring audio pools, setting up preset tag lists, and creating factory objects (timers, broadcasters, preset handlers). The initialization sequence typically follows: fonts -> pool loading -> macro setup -> factory object creation -> UI setup.
- **CPU/peak monitoring dashboard**: FX plugins and instruments alike use `Engine.getCpuUsage()` and `Engine.getMasterPeakLevel()` in timer-driven polling loops to build real-time diagnostic displays. This is one of the most common timer patterns.
- **Unit conversion layer for DSP parameters**: Synthesizers and sequencer-based instruments use the tempo conversion methods (`getMilliSecondsForTempo`, `getSamplesForQuarterBeats`) to synchronize script behavior to host transport, and the musical conversion methods (`getFrequencyForMidiNoteNumber`, `getDecibelsForGainFactor`) for parameter display formatting.
- **Keyboard zone visualization**: Sampler instruments with multiple microphone positions, split key ranges, or chord detection use `Engine.setKeyColour()` in loops to colour-code keyboard zones, providing visual feedback about which notes are mapped, which are active, or what chord is being played.

### Complexity Tiers
1. **Basic utilities** (most common): `getSampleRate`, `getOS`, `isPlugin`, `loadFontAs`, `setGlobalFont`, `loadAudioFilesIntoPool`, `getFrequencyForMidiNoteNumber`, `getDecibelsForGainFactor`. Every plugin uses some subset of these for initialization and basic math.
2. **Factory creation**: `createTimerObject`, `createBroadcaster`, `createUserPresetHandler`, `createMidiList`, `createTransportHandler`, `createMacroHandler`. These methods are the entry points for the major subsystems -- most intermediate plugins use at least timers and preset handlers.
3. **Complex data and rendering**: `createFixObjectFactory`, `createUnorderedStack`, `createMessageHolder`, `createBackgroundTask`, `renderAudio`, `createModulationMatrix`, `createDspNetwork`. These serve advanced architectures: granular synthesis engines, offline audio rendering, dynamic modulation routing, and scriptnode integration.

### Practical Defaults
- Use `Engine.loadFontAs("{PROJECT_FOLDER}Fonts/MyFont.ttf", "MyFont")` with an explicit font ID rather than relying on the font's internal name. This avoids cross-platform font resolution issues.
- Call `Engine.loadAudioFilesIntoPool()` in `onInit` for any plugin that references audio files from scripts. In compiled plugins, this ensures the pool references are available.
- Use `Engine.createTransportHandler()` instead of `Engine.getPlayHead()` for host transport information. The playhead object's properties are not populated.
- When using `Engine.setKeyColour()`, iterate the full 0-127 range and set every key -- including unmapped keys to a dimmed colour like `0x99444444`. This prevents stale colours from a previous state.
- Use `Engine.createTimerObject()` with `startTimer(500)` for CPU/peak meter polling. A 500ms interval provides smooth visual updates without unnecessary overhead.

### Integration Patterns
- `Engine.createTransportHandler()` -> `TransportHandler.setOnTransportChange()` -> `Broadcaster.sendSyncMessage()` -- Create a transport handler, connect its transport change callback to a broadcaster, then let the broadcaster drive UI updates (play/stop buttons, tempo display). This decouples transport state from UI components.
- `Engine.createUserPresetHandler()` -> `UserPresetHandler.setPostCallback()` -> broadcaster -- Connect the preset handler's post-load callback to a broadcaster that fans out state updates to all dependent UI components after a preset loads.
- `Engine.createMacroHandler()` -> `MacroHandler.setExclusiveMode()` -> `MacroHandler.setUpdateCallback()` -- Set up macro automation with exclusive mode (one connection per slot at a time) and drive UI updates through a broadcaster.
- `Engine.createFixObjectFactory()` -> `FixObjectFactory.createArray()` + `Engine.createUnorderedStack()` -- Combine typed fixed-layout objects with an unordered stack for tracking active notes in performance-critical audio-thread code (grain engines, resonance models).
- `Engine.addModuleStateToUserPreset()` with a JSON config object -> per-EQ band persistence -- Pass a JSON object with `ID` and `RemovedProperties`/`RemovedChildElements` to selectively persist only the relevant parts of an effect module's state (e.g., EQ band settings without routing matrix or bypass state).

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using `Engine.createTimerObject()` for sample map loading directly in callbacks | Defer sample map loading through a timer with a 50ms polling interval | Loading sample maps synchronously from UI callbacks causes audio thread hitches. Use a timer to compare `lastIndex` vs `currentIndex` and load only when they differ. |
| Calling `Engine.setKeyColour()` only for mapped notes | Set all 128 keys, using a dim colour for unmapped notes | Skipping unmapped keys leaves stale colours from a previous instrument or preset state. |
| `Engine.addModuleStateToUserPreset("MyEQ")` with just a string | Pass a JSON object with `RemovedProperties` to exclude noise like routing matrix and bypass state | Without filtering, the full module ValueTree is saved, bloating presets and causing unexpected state restoration. |
| `Engine.setFrontendMacros(["M1", "M2"])` with too few names | Provide names for all `HISE_NUM_MACROS` slots (default 8) | Excess slots silently receive empty-string names, which appear as blank entries in macro displays. |
