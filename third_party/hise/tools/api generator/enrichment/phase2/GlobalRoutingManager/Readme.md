# GlobalRoutingManager -- Project Context

## Project Context

### Real-World Use Cases
- **DSP network parameter bridge**: A plugin with custom DSP networks (granular engines, envelope processors) uses GlobalRoutingManager cables to expose internal DSP state back to the script layer for visualization and UI feedback. Each cable carries a single normalised value from a `processorId="GlobalCable"` node in the DSP network to a script-side `registerCallback` or timer-polled `getValue` call. This is the most common use case - bridging the DSP/script boundary without Broadcasters.
- **Global modulator routing**: A synthesizer with a modulation matrix creates cables for each global modulator (envelopes, LFOs, velocity, MIDI CC) and connects them via `connectToGlobalModulator`. This allows the modulation matrix UI to bind sources to targets through the cable system rather than direct module references.
- **Multi-parameter envelope control**: A custom envelope editor UI uses a bank of related cables (attack, decay, sustain, retrigger) to send normalised slider values into DSP network parameters. The script creates all cables from a single manager instance and routes UI changes through `setValueNormalised`.

### Complexity Tiers
1. **Simple value bridge** (most common): `getCable` + `getValue`/`registerCallback` for reading DSP network output in script. Requires only the manager and one or more cables.
2. **Bidirectional parameter control**: `getCable` + `setValueNormalised`/`registerCallback` for two-way communication between UI controls and DSP parameters. Add cable arrays for related parameter groups.
3. **Global modulator integration**: `getCable` + `connectToGlobalModulator` for routing modulators through the cable system into a modulation matrix.
4. **OSC integration**: `connectToOSC` + `addOSCCallback`/`sendOSCMessage` for external controller communication. Requires OSC-addressable cable IDs (starting with `/`).

### Practical Defaults
- Use descriptive cable names that identify what value they carry (e.g., `"EnvelopeValue"`, `"PeakLevel"`). Reserve `/`-prefixed names for cables that need OSC routing.
- For DSP network integration, the cable name in script must exactly match the `processorId` property of the `GlobalCable` node in the scriptnode network.
- When creating a bank of related cables (e.g., envelope parameters), store them in an array indexed to match the corresponding UI components for clean callback routing.

### Integration Patterns
- `GlobalRoutingManager.getCable()` -> `GlobalCable.registerCallback()` -- Read DSP network output values in script for UI visualization (peak meters, envelope displays).
- `GlobalRoutingManager.getCable()` -> `GlobalCable.setValueNormalised()` -- Push UI control values into DSP network parameters via the cable system.
- `GlobalRoutingManager.getCable()` -> `GlobalCable.connectToGlobalModulator()` -- Wire global modulators into a modulation matrix through the cable routing system.
- `GlobalRoutingManager.getCable()` -> `GlobalCable.registerDataCallback()` -- Receive structured data from a scriptnode cable's data channel for complex visualizations (envelope shape data).

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a separate `Engine.getGlobalRoutingManager()` call for each cable | Call `Engine.getGlobalRoutingManager()` once and reuse the reference | Each call creates a new wrapper object. Store the manager in a `const var` and call `getCable` multiple times on it. |
| Using different cable name strings in script vs. DSP network | Ensure the cable ID in `getCable("name")` exactly matches the scriptnode `processorId` | Cable names are matched by exact string equality. A mismatch silently creates a disconnected cable. |
