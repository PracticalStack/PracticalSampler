# ExpansionHandler -- Project Context

## Project Context

### Real-World Use Cases
- **Content expansion delivery**: A sampler or synthesizer ships with a base instrument and delivers additional content (sample maps, audio files, images, MIDI files) as expansion packs. The ExpansionHandler builds a selector UI, switches the active expansion at runtime, and updates all resource references (images, sample maps) through the expansion callback. This is the dominant use case for the class.
- **Expansion close/deactivate button**: A plugin that loads as an expansion inside a shell product provides a close button that calls `setCurrentExpansion("")` to return to the base product state. This is a minimal but common pattern using only two methods.

### Complexity Tiers
1. **Browsing and switching** (most common): `Engine.createExpansionHandler()`, `getExpansionList()`, `setCurrentExpansion()`, `setExpansionCallback()`. Build a selector ComboBox, react to expansion changes by updating UI resources.
2. **Resource enumeration**: Add `Expansion.getSampleMapList()`, `getAudioFileList()`, `getImageList()`, `getMidiFileList()`, `getWildcardReference()` to browse and load expansion-specific content into samplers, audio players, and image components.
3. **Installation pipeline**: Add `installExpansionFromPackage()`, `setInstallCallback()`, `getMetaDataFromPackage()`, `getExpansionForInstallPackage()` for in-app expansion installation with progress tracking.
4. **Encrypted distribution**: Add `setCredentials()`, `encodeWithCredentials()`, `setAllowedExpansionTypes()` for credential-encrypted expansion delivery in production builds.

### Practical Defaults
- Always set `setExpansionCallback()` before the first call to `setCurrentExpansion()` so the callback fires on the initial switch.
- Call the expansion callback manually with `undefined` at init time to set the default (no-expansion) UI state. This ensures the UI is correct before any expansion is loaded.
- Pass an empty string to `setCurrentExpansion("")` to deactivate the current expansion and return to the base product state.
- Use `setAllowedExpansionTypes([eh.Intermediate, eh.Encrypted])` in production builds to hide FileBased development folders from end users.

### Integration Patterns
- `ExpansionHandler.getExpansionList()` -> `Expansion.getSampleMapList()` -> `Sampler.loadSampleMap()` -- Build a two-column browser: left column lists expansions (plus a "Root" entry for embedded maps), right column lists the selected expansion's sample maps.
- `ExpansionHandler.setExpansionCallback()` -> `Expansion.getWildcardReference()` -> `ScriptImage.set("fileName", ...)` -- Swap background images and panel graphics when the active expansion changes.
- `ExpansionHandler.getExpansionList()` -> `Expansion.getAudioFileList()` -> `AudioSampleProcessor.setFile()` -- Aggregate audio files from all expansions into a single selector for audio loop players.
- `ExpansionHandler.setCurrentExpansion("")` -- Wire to a close button to deactivate the current expansion and restore the base product state.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Setting up the expansion callback after `setCurrentExpansion()` | Set `setExpansionCallback()` first, then call `setCurrentExpansion()` | The callback won't fire for the initial switch if it hasn't been registered yet. |
| Assuming the callback argument is always valid | Always check `isDefined(e)` in the expansion callback | The callback receives `undefined` when the expansion is cleared via `setCurrentExpansion("")`. |
| Not initializing UI for the no-expansion state | Call `refreshCallback(undefined)` at init | The expansion callback only fires on changes. Without an explicit init call, the UI may show stale or empty state until the user selects an expansion. |
