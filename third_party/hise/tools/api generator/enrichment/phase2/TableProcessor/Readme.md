# TableProcessor -- Project Context

## Project Context

### Real-World Use Cases
- **Dynamics crossfade curves**: A sampler with multiple velocity or dynamics layers uses TableProcessor to programmatically build equal-power or S-shaped crossfade curves on ControlModulator or VelocityModulator tables. Each layer gets a complementary curve segment so that the combined output maintains constant perceived loudness across the crossfade range.
- **Key range gating**: A multi-oscillator synthesiser uses KeyModulator tables to define per-oscillator playable ranges. The table is shaped as a bandpass gate (0 outside the range, 1 inside) so that each oscillator only sounds within its assigned key zone.
- **Articulation envelope shaping**: An instrument with multiple articulations (sustain, staccato, legato) dynamically reshapes table envelope curves when the player switches articulation. The table envelope's attack shape is rebuilt via reset-add-set sequences to match the expected envelope contour for each playing style.

### Complexity Tiers
1. **Simple curve adjustment** (most common): `reset()` + `setTablePoint()` on the two default edge points. Sufficient for configuring a basic fade-in or fade-out shape on a single table.
2. **Multi-point curve building**: `reset()` + `addTablePoint()` + `setTablePoint()` to create curves with interior points. Used for bandpass shapes, crossfade segments, and custom envelope contours.
3. **Programmatic multi-table generation**: Operating on multiple tables (tableIndex > 0) across arrays of TableProcessor references, building complementary curve sets for crossfade layers or per-note parameter mapping.

### Practical Defaults
- Use `reset()` before any sequence of `addTablePoint()` calls to ensure a clean starting state. Every project that builds custom curves follows this pattern.
- Use curve value `0.5` for linear interpolation, `0.25` for concave (equal-power fade-out), and `0.75` for convex (equal-power fade-in) segments.
- Most modules have a single table at index 0. Use `tableIndex: 1` only when the module explicitly provides a second table (e.g., KeyModulator has separate tables for different mapping targets).
- When adjusting an existing table shape at runtime (e.g., from a knob callback), call `setTablePoint()` directly on known point indices rather than rebuilding the entire table with reset-add-set.

### Integration Patterns
- `Synth.getTableProcessor(id)` -> `TableProcessor.reset()` -> `TableProcessor.addTablePoint()` -> `TableProcessor.setTablePoint()` -- the standard curve-building sequence, always starting from a clean state.
- `Synth.getModulator(id)` -> `Modulator.asTableProcessor()` -> `TableProcessor.setTablePoint()` -- used when the modulator reference already exists and table access is needed for dynamic curve adjustment (e.g., in articulation switching or control callbacks).
- `Synth.getTableProcessor("SamplerName")` -> `TableProcessor.reset()` + `TableProcessor.setTablePoint()` -- accessing a ModulatorSampler's crossfade table to enable/configure dynamics crossfading.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `addTablePoint()` without `reset()` first | Always call `reset()` before building a new curve shape | Without reset, new points accumulate on top of existing ones, producing an unpredictable curve. Every curve-building sequence should start with `reset()`. |
| Calling `asTableProcessor()` repeatedly in a callback | Store the TableProcessor reference in a `const var` at init time, or at minimum cache the result | `asTableProcessor()` creates a new wrapper object each call. Cache the reference when possible. |
