# UserPresetHandler -- Project Context

## Project Context

### Real-World Use Cases
- **Multi-channel instrument with structured preset data**: A plugin with many channels (e.g., a drum machine with per-channel layers, effects, and mix parameters) generates hundreds of automation slots. The default component-per-slot model becomes impractical because the parameter space is structured (channel > layer > parameter) but the default preset format is flat. The custom data model (`setUseCustomUserPresetModel`) bypasses this limitation by letting the save/load callbacks serialize arbitrary structured JSON, while `setCustomAutomation` maps the structured parameters to DAW-visible automation slots.
- **FX plugin with preset migration and parameter locking**: An FX plugin uses `setEnableUserPresetPreprocessing` and `setPreCallback` to migrate presets from older versions - adding missing controls, renaming changed parameters, and adjusting value ranges. The post-callback drives a broadcaster that updates the entire UI after a preset change. `isInternalPresetLoad` distinguishes DAW state recall from user-initiated preset changes, allowing "parameter lock" features where certain values survive preset changes unless the load comes from a DAW session restore.
- **Lightweight preset lifecycle hooks**: Any plugin that needs to run logic before or after preset loads (e.g., updating a display, syncing UI state, resetting transport) uses `setPreCallback` and `setPostCallback` without adopting the full custom data model.

### Complexity Tiers
1. **Basic lifecycle** (most common): `setPostCallback` to update UI after preset load. No custom data model needed - the default `saveInPreset` component serialization handles everything. Methods: `setPostCallback`, optionally `setPreCallback`.
2. **Preset migration**: Add `setEnableUserPresetPreprocessing` and `setPreCallback` to inspect and modify preset data before loading. Use `isOldVersion` to detect presets from older plugin versions. Methods add: `setEnableUserPresetPreprocessing`, `setPreCallback`, `isOldVersion`.
3. **Custom data model with automation**: Enable `setUseCustomUserPresetModel` for full control over preset serialization, then `setCustomAutomation` to define DAW-visible parameter slots with processor/cable/meta connections. Methods add: `setUseCustomUserPresetModel`, `setCustomAutomation`, `attachAutomationCallback`, `setAutomationValue`, `getAutomationIndex`, `updateAutomationValues`, `createObjectForAutomationValues`, `createObjectForSaveInPresetComponents`, `updateSaveInPresetComponents`.
4. **Host integration polish**: Add parameter gesture tracking (`setParameterGestureCallback`, `sendParameterGesture`), parameter grouping (`setPluginParameterGroupNames`), sort order control (`setPluginParameterSortFunction`), and undo support (`setUseUndoForPresetLoading`).

### Practical Defaults
- Use `setPostCallback` as the first entry point for preset-aware logic. It runs asynchronously on the message thread after the full load completes, making it safe for UI updates.
- Use `setEnableUserPresetPreprocessing(true, false)` for version migration. Only pass `true` for the second parameter (`shouldUnpackComplexData`) when you need to inspect Base64-encoded data inside the preset (e.g., migrating sample map references).
- Pass a Broadcaster directly to `setPreCallback` or `setPostCallback` instead of a plain function when multiple systems need to react to preset changes. This turns the preset lifecycle into an event bus that multiple listeners can subscribe to independently.
- Use `SyncNotification` for `attachAutomationCallback` dispatch when the callback drives audio-thread state (e.g., updating `reg` variables). Use `AsyncNotification` when the callback updates UI (e.g., repainting a panel to reflect the new value).
- Call `setUseCustomUserPresetModel` before `setCustomAutomation` - the custom data model is a prerequisite.
- Set `allowHostAutomation: false` on per-layer automation slots that should not clutter the DAW's parameter list. Reserve `allowHostAutomation: true` for the most important user-facing parameters.

### Integration Patterns
- `UserPresetHandler.setPostCallback()` -> `Broadcaster.sendAsyncMessage()` - Pass a broadcaster as the post-callback to fan out preset-change notifications to multiple UI listeners without coupling them to the preset handler.
- `UserPresetHandler.setPreCallback()` -> `Broadcaster.sendSyncMessage()` - Same pattern for pre-load: a broadcaster as the pre-callback lets multiple systems (FX lock, transport stop, UI preparation) react independently before the preset loads.
- `UserPresetHandler.attachAutomationCallback()` -> `ScriptPanel.repaint()` - Attach async automation callbacks to drive custom panel displays (XY pads, meters, visualizers) that reflect automation values in real time.
- `UserPresetHandler.isInternalPresetLoad()` -> conditional restore logic - Check inside pre/post callbacks to distinguish DAW state recall from user preset changes, enabling "parameter lock" features where locked values are preserved on user-initiated preset changes but overwritten on DAW recall.
- `UserPresetHandler.getSecondsSinceLastPresetLoad()` -> debounce guard - Use the elapsed time since last preset load to suppress side effects that would conflict with an in-progress preset restore (e.g., skip cleanup operations that fire within 500ms of a preset load).
- `UserPresetHandler.createObjectForAutomationValues()` / `updateAutomationValues()` -> round-trip automation state - Capture all automation values as a snapshot array, then restore from the snapshot. Used in custom save/load callbacks and channel copy/paste operations.
- `UserPresetHandler.updateConnectedComponentsFromModuleState()` -> sync UI after programmatic module changes - Call after restoring module bypass states or other processor attributes to push the new values back into connected UI components.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using a plain function as the post-callback when many systems need to react | Pass a Broadcaster to `setPostCallback` and add multiple listeners | A single callback function forces all preset-change reactions into one place. Passing a broadcaster decouples the preset handler from individual UI systems and allows each to register independently. |
| Calling `updateAutomationValues` with a single object `{"id": "Vol", "value": 0.5}` | Wrap in an array: `[{"id": "Vol", "value": 0.5}]` | The method expects an Array, not a single object. Passing an unwrapped object throws a script error. |
| Checking `isInternalPresetLoad()` outside of a pre/post callback | Only call `isInternalPresetLoad()` inside `setPreCallback` or `setPostCallback` | Outside those callbacks, the flag retains its value from the most recent load, which may be stale and produce incorrect conditional logic. |
| Setting `allowHostAutomation: true` on every automation slot in a large instrument | Set `allowHostAutomation: false` on per-layer/internal slots; only expose top-level parameters to the host | Exposing hundreds of parameters to the DAW creates an unusable automation list. Reserve host automation for the parameters users are most likely to automate. |
