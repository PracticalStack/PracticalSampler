# Settings -- Project Context

## Project Context

### Real-World Use Cases
- **UI zoom control**: Every shipping plugin needs a zoom mechanism. The two dominant patterns are a drag-to-zoom corner handle (a ScriptPanel that converts mouse drag distance into zoom steps) and a ComboBox with discrete zoom levels. Both use `getZoomLevel`/`setZoomLevel` and clamp to screen bounds.
- **Standalone application setup**: Standalone builds use Settings to auto-detect and enable MIDI input devices at first launch, so the user can play immediately without manual device configuration.
- **Graphics quality management**: Plugins with GPU-rendered visuals tie `setEnableOpenGL`/`isOpenGLEnabled` to a quality selector (Off/Low/Medium/High), persisting the choice to an XML file in AppData. The deferred nature of OpenGL toggling requires a "please reload" message.
- **Sample folder relocation**: Plugins with external sample content provide a settings menu or missing-content dialog that browses for a directory, validates it contains the expected subfolder structure, then calls `setSampleFolder` to persist the new location.
- **Performance profiling**: Development builds embed a hidden Perfetto toggle (e.g., logo click) that starts/stops tracing and opens the resulting `.pftrace` file on the desktop.

### Complexity Tiers
1. **Zoom control** (most common): `getZoomLevel`, `setZoomLevel`, plus `Content.getScreenBounds` for screen-aware clamping. Nearly every plugin implements this.
2. **Standalone configuration**: Audio device methods (`setAudioDevice`, `setBufferSize`, `setSampleRate`) and MIDI input methods (`getMidiInputDevices`, `toggleMidiInput`). Only needed for standalone builds.
3. **Advanced settings**: `setSampleFolder` for sample relocation, `setEnableOpenGL` for graphics quality tiers, Perfetto tracing for development profiling.

### Practical Defaults
- Use 0.25 zoom step increments for drag-to-zoom. This gives clean values (0.75, 1.0, 1.25, 1.5...) that feel natural.
- Clamp zoom to screen bounds using `Content.getScreenBounds(false)[3] / interfaceHeight` as the maximum, not just the Settings 2.0 ceiling.
- After toggling OpenGL with `setEnableOpenGL`, always show a message box telling the user to reload the plugin - the change does not take effect until the next interface rebuild.
- In standalone first-run installers, auto-enable all detected MIDI inputs so the user can play immediately.

### Integration Patterns
- `Settings.setZoomLevel()` with `Content.getScreenBounds()` - screen dimensions provide the upper zoom bound before calling setZoomLevel.
- `Settings.setSampleFolder()` with `FileSystem.browseForDirectory()` - the file browser produces the File object that setSampleFolder requires.
- `Settings.stopPerfettoTracing()` with `FileSystem.getFolder(FileSystem.Desktop).getChildFile()` - trace files are conventionally saved to the desktop.
- `Settings.toggleMidiInput()` with `Settings.getMidiInputDevices()` - enumerate all devices, then enable each one in a loop.
- `Settings.setZoomLevel()` with `Broadcaster` - advanced architectures route zoom changes through a Broadcaster so multiple UI elements can respond (e.g., updating a ComboBox indicator while applying the zoom).

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Settings.setZoomLevel(newZoom);` without screen bounds check | Clamp to `Content.getScreenBounds(false)[3] / interfaceHeight` first | The interface can grow larger than the screen. Always compute the maximum zoom from the display size before applying. |
| Calling `Settings.setEnableOpenGL(true)` and expecting immediate effect | Show a reload message after toggling | OpenGL context changes are deferred until the next interface rebuild. The UI will not change until the plugin is reloaded. |
| Passing a string path to `Settings.setSampleFolder("/path")` | Pass a `File` object from `FileSystem.browseForDirectory()` | setSampleFolder requires a File object. String paths are silently ignored with no error. |
