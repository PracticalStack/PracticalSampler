# MatrixModulator - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/modulators/mods/MatrixModulator.h` (~200 lines)
- `hi_core/hi_modules/modulators/mods/MatrixModulator.cpp` (~924 lines)
- `hi_core/hi_modules/modulators/editors/MatrixModulatorEditor.h` (editor)
- `hi_core/hi_modules/modulators/editors/MatrixModulatorComponents.h` (UI components)

**Base class:** `EnvelopeModulator` + `ExternalDataHolder` + `ModulatorSynthChain::Handler::Listener`

## Signal Path

MatrixModulator combines multiple global modulators into a single output via a matrix-style connection system. It does NOT use the `GlobalModulator` base class - it has a completely separate connection mechanism based on the runtime target system.

1. User creates connections via the MatrixContent table UI (or drag-and-drop)
2. Each connection has: source modulator, mode (Scale/Add/Bipolar), intensity, optional auxiliary source
3. On `startVoice()`: smoothed base value is reset, all connected mods process the voice-start event
4. On `calculateBlock()`:
   a. Buffer filled with smoothed `baseValue` (per-sample interpolation if ramping)
   b. Scale mods are processed (multiplicative against base)
   c. Add mods are processed (additive offsets)
   d. Output range skew applied if non-default
5. Result: `output = (baseValue * scaleMod1 * scaleMod2 ...) + addMod1 + addMod2 ...`

## Gap Answers

### signal-flow

**Question:** How does MatrixModulator combine multiple global modulators?

**Answer:** Connections are separated into `scaleMods` (multiplicative) and `addMods` (additive) in `rebuildModList()`. In `calculateBlock()`, the buffer is first filled with the smoothed base value, then scale mods are processed sequentially (each multiplies the buffer), then add mods are processed (each adds to the buffer). Each connection wraps a `scriptnode::core::matrix_mod<NUM_POLYPHONIC_VOICES>` node. The combination formula is: `output = (baseValue * scaleMod1 * ... * scaleModN) + addMod1 + ... + addModM`.

### global-modulator-sources

**Question:** How are source modulators selected and connected?

**Answer:** Sources are selected through the `MatrixContent` table UI, which provides rows for each connection. Each row has a ComboBox to select the source modulator from the container's gain chain. Connections are stored as children of a `MatrixData` ValueTree in the container's `RuntimeSource`. New connections can also be created via drag-and-drop (`onModulationDrop()`) or right-click popup menu (`GlobalContainerMatrixModulationPopupData`).

Each connection has:
- Source selector (ComboBox picking from container's modulators)
- Mode: Scale (multiplicative), Add (additive), or Bipolar (additive with bipolar range)
- Intensity slider [-1..1]
- Optional auxiliary source with its own intensity
- Per-connection inversion toggle

### value-param-role

**Question:** What role does the Value parameter play?

**Answer:** Value is the **base value** of the modulation output. It serves as the starting point before any modulator connections are applied. The Value goes through `inputRange` normalisation to [0..1], then becomes the `baseValue` (a smoothed float). In `calculateBlock()`, the buffer is first filled with this smoothed base value, then scale mods multiply against it and add mods offset from it. When all connections are removed, the modulator auto-bypasses and outputs just this base value. When bypassed, the parent chain's initial value is set from the base value.

### smoothing-detail

**Question:** How does the smoother work?

**Answer:** The `baseValue` field is an `sfloat` (smoothed float), prepared at control rate: `sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`. The SmoothingTime parameter controls the ramp duration (default 50ms, range 0-2000ms). In `calculateBlock()`, if `baseValue.isActive()` (still ramping), `baseValue.advance()` is called per sample. Otherwise, the buffer is filled with the constant `baseValue.get()`. On voice start, `baseValue.reset()` snaps to the target immediately. When SmoothingTime is 0, transitions are instant.

### monophonic-retrigger

**Question:** Are Monophonic and Retrigger functional?

**Answer:** Both are functional. The `reset()` method has explicit monophonic awareness: in monophonic mode, it skips resetting the inner modulation nodes to maintain continuous signal. `isPlaying()` delegates to the inner `matrix_mod` nodes when scale envelopes are present. Retrigger is handled by the base EnvelopeModulator framework.

### disconnected-behavior

**Question:** What happens when no global modulators are connected?

**Answer:** If no GlobalModulatorContainer exists, `container` is null and `init()` returns early. The modulator outputs its smoothed `baseValue` with no modulation. When all items (connections) are removed, the modulator auto-bypasses itself.

## Processing Chain Detail

1. **startVoice** (low) - reset smoothed base value, iterate all mods for voice-start processing
2. **calculateBlock** (medium) - per-sample smoothing, sequential processing of scale and add mods, optional output range skew
3. **isPlaying** (negligible-low) - iterates mods when scale envelopes present
4. **reset** (negligible) - monophonic-aware reset of inner nodes

## Modulation Points

No modulation chains of its own. Receives values from connected global modulators via the runtime target system and `matrix_mod` DSP nodes.

## Conditional Behaviour

- **Connection mode**: Scale mods multiply, Add mods add, Bipolar acts as add with bipolar range display
- **Auto-bypass**: When all connections removed, modulator bypasses itself
- **Monophonic mode**: Skips inner node reset to maintain continuous signal
- **Output range**: When non-default, applies skew factor per-sample

## CPU Assessment

**Medium baseline cost, scaling with connections.** Per-sample smoothing of base value, plus per-sample processing for each connected mod. Output range skew adds per-sample overhead. Cost scales linearly with the number of active connections. Lock acquisition (`SimpleReadWriteLock::ScopedTryReadLock`) each block adds minor overhead.

## UI Components

- `MatrixModulatorBody` editor with Value slider, Smoothing slider, and MatrixContent table
- `MatrixContent` provides the connection table (rows with source/mode/intensity/aux/plotter/delete)
- `MatrixContent::Controller` provides drag buttons for creating connections
- `RangeEditor` for input/output range editing
- `ModulationMatrixPanel` and `ModulationMatrixControlPanel` FloatingTile panels
- `TextEditor targetIdEditor` for custom target ID

## Notes

MatrixModulator is architecturally distinct from the other Global*Modulator consumers. It does not inherit `GlobalModulator` and uses the runtime target system rather than string-based lookup. It provides a much richer connection model with per-connection mode, intensity, auxiliary sources, and inversion. The auto-bypass behaviour when all connections are removed is a useful optimisation. The separation of scale (multiplicative) and add (additive) processing is a deliberate design choice that gives predictable combination behaviour.
