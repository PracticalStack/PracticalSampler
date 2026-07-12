# ScriptAudioWaveform -- Project Context

## Project Context

### Real-World Use Cases
- **One-shot sample player with waveform display**: A drum machine or sampler-based plugin uses ScriptAudioWaveform to show the currently loaded audio sample per channel. The waveform dynamically re-binds to different processors when the user switches between channels, and the colour updates to match each channel's identity. A LAF overrides all six thumbnail draw functions for fully custom rendering.
- **IR reverb file browser**: An FX plugin uses ScriptAudioWaveform to display a convolution reverb's impulse response. The component connects to an `AudioSampleProcessor` via `processorId` and uses a Broadcaster attached to the complex data slot (`attachToComplexData("AudioFile.Content", ...)`) to automatically repaint when the IR changes.

### Complexity Tiers
1. **Basic display** (most common): Create the waveform, connect via `processorId`, optionally configure `enableRange`, `showFileName`, and `opaque`. No custom LAF needed.
2. **Channel-switching display**: Dynamically reassign `processorId` at runtime when the user switches channels, update colours via `set("itemColour", ...)`, and call `setPlaybackPosition(0)` to reset the cursor.
3. **Fully custom rendering**: Register all six LAF draw functions (`drawThumbnailBackground`, `drawThumbnailPath`, `drawThumbnailRuler`, `drawThumbnailRange`, `drawThumbnailText`) plus `getThumbnailRenderOptions` to completely control the visual appearance. Use `getRangeEnd()` to coordinate envelope overlay positioning with sample length.

### Practical Defaults
- Set `opaque` to `false` and `bgColour` to `0` (transparent) when layering the waveform over a custom-painted ScriptPanel background.
- Set `enableRange` to `false` for display-only waveforms where user range selection is not needed.
- Set `showFileName` to `false` when using a custom LAF that handles text display differently.
- Use `getThumbnailRenderOptions` to set `forceSymmetry` to `true` for one-shot drum samples where a symmetric waveform looks cleaner.
- Use `getThumbnailRenderOptions` to set `scaleVertically` to `true` for IR displays where the absolute amplitude is less important than the shape.

### Integration Patterns
- `Broadcaster.attachToComplexData("AudioFile.Content", processorId, index)` -> `Broadcaster.addComponentRefreshListener(waveformId, "repaint")` -- Automatically repaint the waveform when the audio file changes in the connected processor, without polling.
- `Broadcaster.attachToComponentValue(components)` -> `wf.set("processorId", newId)` + `wf.setPlaybackPosition(0)` -- When a channel selector changes, rebind the waveform to a different audio processor and reset the playback cursor.
- `wf.getRangeEnd()` -> envelope path calculation -- Use the total sample count from the waveform's range to convert envelope times (in samples) to normalized path coordinates for an overlay.
- `Broadcaster.addComponentPropertyListener(waveformId, ["itemColour", "itemColour2"])` -- Drive waveform colour changes from a broadcaster instead of manual `set()` calls, keeping channel-selection logic centralized.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `set("processorId", newId)` without resetting the playback cursor | `set("processorId", newId)` followed by `setPlaybackPosition(0)` | When rebinding to a different processor, the playback cursor position is stale from the previous audio file. Reset it to avoid a cursor stuck at an arbitrary position. |
| Polling for audio file changes with a Timer | Using `Broadcaster.attachToComplexData("AudioFile.Content", ...)` with `addComponentRefreshListener` | Broadcasters provide event-driven repaint triggers without the overhead and latency of timer-based polling. |
| Setting only `itemColour` when changing channel colour | Setting both `itemColour` (outline) and `itemColour2` (fill) | The waveform uses two colour properties for its visual appearance. Changing only one produces a mismatched look. |
