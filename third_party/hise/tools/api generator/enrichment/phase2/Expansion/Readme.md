# Expansion -- Project Context

## Project Context

### Real-World Use Cases
- **Expansion-aware content browser**: A sampler plugin that ships expansion packs uses Expansion to list each pack's sample maps, images, and audio files, building a two-column browser UI that lets users select content across all installed expansions. The left column shows expansions (plus a "Root" entry for embedded content), and the right column shows the selected expansion's resources.
- **Themed UI per expansion**: A plugin that visually rebrands per expansion pack uses `getWildcardReference()` to load expansion-specific background images and panel graphics, falling back to `{PROJECT_FOLDER}` references when no expansion is active. The expansion callback from `ExpansionHandler.setExpansionCallback()` receives the Expansion object and triggers the image swap.
- **Unified resource aggregation**: A plugin with expansion-based audio content iterates all installed expansions via `getAudioFileList()` or `getMidiFileList()`, collecting every resource into a single flat list for a unified file selector. The wildcard reference strings from each expansion are used directly as pool references for loading.

### Complexity Tiers
1. **Resource listing** (most common): `getProperties()`, `getSampleMapList()`, `getAudioFileList()`, `getImageList()`, `getMidiFileList()`. Read metadata and enumerate available content per expansion.
2. **Resource referencing**: `getWildcardReference()`. Build `{EXP::Name}path` pool references for loading expansion resources into UI components or audio processors.
3. **Data persistence**: `loadDataFile()`, `writeDataFile()`, `getDataFileList()`. Store and retrieve per-expansion JSON configuration in the AdditionalSourceCode folder.
4. **Advanced management**: `setSampleFolder()`, `rebuildUserPresets()`, `setAllowDuplicateSamples()`, `unloadExpansion()`, `getExpansionType()`. Manage expansion filesystem redirection, preset extraction, sample deduplication, and lifecycle.

### Practical Defaults
- Use `getProperties().Name` to display expansion names in combo boxes and browser UIs.
- Use `getWildcardReference()` to build image paths for expansion-specific theming; fall back to `{PROJECT_FOLDER}` references when no expansion is active.
- Combine `getSampleMapList()` from each expansion with `Sampler.getSampleMapList()` for embedded content to build a complete sample map browser.
- Iterate all expansions in `onInit` to build resource lists; use `ExpansionHandler.setExpansionCallback()` to react to expansion changes at runtime.

### Integration Patterns
- `Expansion.getSampleMapList()` -> `Sampler.loadSampleMap()` -- Load a sample map from a specific expansion into a sampler module.
- `Expansion.getWildcardReference()` -> `ScriptImage.set("fileName", ref)` -- Set expansion-specific images on UI components using the wildcard pool reference.
- `Expansion.getAudioFileList()` -> `AudioSampleProcessor.setFile()` -- Load expansion audio files into AudioLoopPlayer slots.
- `Expansion.getProperties().Name` -> `ExpansionHandler.setCurrentExpansion()` -- Switch the active expansion by name from a combo box selection.
- `Expansion.getMidiFileList()` -> `MidiPlayer.setFile()` -- Load expansion MIDI files into a MIDI Player module.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using expansion resources without handling the "no expansion" case | Check `isDefined(expansion)` and fall back to `{PROJECT_FOLDER}` references | The expansion callback receives `undefined` when no expansion is active. UI code that assumes a valid Expansion object will fail. |
| Building resource lists only from expansions | Include `Sampler.getSampleMapList()` or `Engine.loadAudioFilesIntoPool()` for embedded root content | Expansions supplement the project's own resources. A browser that only shows expansion content hides the base product's sample maps and audio files. |
