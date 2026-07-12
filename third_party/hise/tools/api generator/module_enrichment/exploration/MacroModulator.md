# MacroModulator - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/modulators/mods/MacroControlModulator.h` (142 lines)
- `hi_core/hi_modules/modulators/mods/MacroControlModulator.cpp` (250 lines)

## Gap Answers

### signal-flow

**Question:** What is the complete signal flow from macro value input to modulation output?

**Answer:** The signal flow is:

1. The macro control system calls `macroControllerMoved(newValue)` (line 186) when the linked macro slot changes.
2. The input value is clamped to [0, 1].
3. If UseTable is enabled, the value is passed through the lookup table via `getTableUnchecked()->getInterpolatedValue()` (line 192). If disabled, the raw value is used directly.
4. The result is stored as `targetValue`.
5. In `calculateBlock()` (line 213), the smoother interpolates `currentValue` towards `targetValue`:
   - If smoothing is active (target != current), it processes sample-by-sample via `smoother.smooth(targetValue)` (line 221).
   - If already converged, it fills the buffer with a constant value via `FloatVectorOperations::fill()` (line 230).
6. The buffer contents become the modulation output.

The table lookup happens before smoothing, so the smoother operates on post-table values.

### smoothing-detail

**Question:** How does the smoother work? What rate does it operate at?

**Answer:** The `Smoother` class is prepared with `getControlRate()` (line 208), which is `sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR`. However, in `calculateBlock()`, the smoother is called per-sample in a while loop (lines 219-224), so it effectively produces sample-accurate output at the control rate resolution.

The smoothing time defaults to 200ms (constructor line 73). When SmoothTime is 0, the smoother converges instantly - `smoother.smooth(target)` returns the target immediately when the smoothing coefficient is zero.

The smoothing check at line 215 uses `FloatSanitizers::isNotSilence(targetValue - currentValue)` - essentially checking if the difference is above a noise floor threshold. When the difference is negligible, the block is filled with a constant value (no per-sample processing), which is a CPU optimization.

### table-application

**Question:** Where exactly is the table lookup applied?

**Answer:** The table lookup is applied in `macroControllerMoved()` (line 190-193), which is called by the macro control system whenever the macro value changes. This happens outside the audio callback - it's an event-driven update.

The input is the raw macro value [0, 1]. The output is the table's interpolated value at that position. This post-table value becomes `targetValue`, which the smoother then interpolates towards in `calculateBlock()`.

The table is a standard HISE lookup table (LookupTableProcessor interface) with 0-1 input and 0-1 output. Users can draw custom response curves to reshape the macro control range.

### macro-value-param

**Question:** How does the MacroValue parameter work?

**Answer:** MacroValue is a write-only bridge parameter. In `setInternalAttribute()` (line 162), setting MacroValue calls `macroControllerMoved(newValue)`, which is the same path the macro system uses. In `getAttribute()` (line 149), reading MacroValue triggers `jassertfalse` (debug assertion failure) and returns a hardcoded 1.0.

This parameter exists so the macro control system can drive the modulator through the standard `setAttribute()` mechanism. It is registered as a controlled parameter via `addControlledParameter()` (line 136) when a macro index is assigned. Users should never interact with this parameter directly.

### disconnected-behavior

**Question:** What happens when MacroIndex is -1 (disconnected)?

**Answer:** When MacroIndex is -1 (the default), the modulator is not connected to any macro slot. The constructor initializes `targetValue = 1.0f` and `currentValue = 1.0f` (lines 70-71), so the modulation output is a constant 1.0 (full pass-through in gain mode).

When `addToMacroController(-1)` is called (line 116-137), it removes the modulator from all macro control data lists but does not reset the current value. So after disconnecting, the modulator retains whatever value it had at the time of disconnection. Only a fresh instance starts at 1.0.

## Additional Findings

- The smoother operates at the control rate but `calculateBlock()` processes sample-by-sample when smoothing is active, giving smooth per-sample output.
- `handleHiseEvent()` (line 200) is empty - this modulator does not respond to MIDI events.
- No child processors at all (`getNumChildProcessors()` returns 0).
- The editor uses a combo box for macro slot selection (MacroControlModulatorEditorBody).

## Issues

No description inaccuracies found. All base data descriptions match the C++ implementation.
