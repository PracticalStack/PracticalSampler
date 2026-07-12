# ScriptModulationMatrix -- Project Context

## Project Context

### Real-World Use Cases
- **Wavetable synthesizer with full modulation routing**: A dual-oscillator wavetable synth uses the modulation matrix to route 11 sources (4 AHDSR envelopes, LFO, MIDI CC, Velocity, Pitch Wheel, Table Envelope, Ramp, Random) to 16 targets across two oscillator groups (Gain, Pitch, Fine Tune, Wavetable Position, Detune, Filter Frequency, Filter Q, Pan per oscillator). The Builder API creates MatrixModulator processors in each target chain, and the built-in FloatingTile panels provide drag-and-drop connection management.
- **Custom modulation matrix (pre-API)**: Before the built-in ScriptModulationMatrix existed, complex synthesizers built fully custom modulation matrices in HiseScript (800+ lines) using `addGlobalModulator()`, `addStaticGlobalModulator()`, and hidden panels for persistence. The built-in system replaces this with a fraction of the code.

### Complexity Tiers
1. **Minimal setup** (most common): Create the matrix with `Engine.createModulationMatrix()`, use built-in FloatingTile panels (`ModulationMatrix` and `ModulationMatrixController`) for the UI, and let the `UserPresetStateManager` handle persistence automatically. This requires only 1-2 lines of scripting API code beyond the module tree setup.
2. **Configured defaults**: Add `setMatrixModulationProperties()` to define per-target default intensities, modulation modes, and range presets. This controls what happens when users create connections via drag-and-drop.
3. **Custom interaction layer**: Register callbacks (`setConnectionCallback`, `setDragCallback`, `setEditCallback`, `setSourceSelectionCallback`) and use `getModulationDisplayData()` to build fully custom modulation UIs with ScriptPanel rendering, context menus, and drag visualization.

### Practical Defaults
- Use the built-in FloatingTile panels as a starting point. The `ModulationMatrixController` panel provides source dragger buttons, and the `ModulationMatrix` panel provides either a table or slider grid view. Custom scripting is only needed for non-standard interaction patterns.
- Range presets like `"FilterFreq"` and `"Gain0dB"` handle the most common target types. Use custom range objects only when the presets do not match the parameter's domain.
- When setting `DefaultInitValues`, always include both `"Intensity"` and `"Mode"` together. A non-zero intensity without a mode causes a script error.

### Integration Patterns
- `Synth.createBuilder()` -> `builder.create(builder.Modulators.MatrixModulator, ...)` -- The Builder API creates MatrixModulator instances in target processor chains during module tree construction. Each MatrixModulator becomes a modulation target.
- `builder.create("GlobalModulatorContainer", ...)` -> `Engine.createModulationMatrix(containerId)` -- The Builder creates the container, then the scripting API wraps it as a matrix object. The container ID passed to `createModulationMatrix` must match the Builder-created container's name.
- `ScriptSlider.set("matrixTargetId", id)` -> `ScriptModulationMatrix.getTargetId(slider)` -- Setting `matrixTargetId` on a slider registers it as a parameter-level modulation target. The matrix discovers these targets automatically.
- `Engine.createBroadcaster()` -> `Broadcaster.attachToContextMenu()` -> `ScriptModulationMatrix.clearAllConnections()` -- A broadcaster-driven context menu provides "Clear all connections" functionality without manual popup handling.
- `ScriptFloatingTile.setContentData({"Type": "ModulationMatrixController", "ProcessorId": containerId})` -- The built-in controller panel connects to the matrix via the container's processor ID.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Building a custom modulation matrix with `addGlobalModulator()` and hidden panels for persistence | Use `Engine.createModulationMatrix()` with built-in FloatingTile panels | The built-in system handles connection state, preset integration, drag-and-drop, and display data automatically. Custom implementations require 800+ lines to replicate this. |
| Setting `matrixTargetId` to the component ID directly | Convert component IDs to target-friendly format (e.g., replace underscores with spaces) | Target IDs must match either a MatrixModulator processor ID or the exact `matrixTargetId` string. Component naming conventions with underscores may not match processor naming with spaces. |
| Creating the matrix object but never placing FloatingTile panels or registering callbacks | Either use FloatingTile panels for the built-in UI or register drag/connection callbacks for custom UI | The matrix object alone does not provide any user-facing interaction. Without panels or callbacks, users have no way to create or manage connections. |
