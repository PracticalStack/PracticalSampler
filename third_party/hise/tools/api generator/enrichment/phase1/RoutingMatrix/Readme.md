# RoutingMatrix -- Class Analysis

## Brief
Script handle for managing internal audio channel routing connections and send connections within a processor.

## Purpose
The `RoutingMatrix` object provides script-level access to a processor's internal audio channel routing. It wraps the C++ `RoutableProcessor::MatrixData` class, exposing methods to add and remove source-to-destination channel connections (including parallel send connections), query channel counts, inspect current peak levels, and resize the matrix. It works with any processor that implements the `RoutableProcessor` interface -- primarily synths (`ModulatorSynth`), polyphonic effects (`VoiceEffectProcessor`), and hardcoded DSP modules. Routing changes are automatically broadcast via `SafeChangeBroadcaster`, enabling reactive UI updates through `Broadcaster.attachToRoutingMatrix`.

## Details

### Connection Model

The matrix maintains two parallel fixed-size arrays (up to 16 channels each):

| Array | Purpose |
|-------|---------|
| `channelConnections` | Primary source-to-destination mapping |
| `sendConnections` | Parallel send routing (independent of primary) |

Each source channel maps to at most one destination channel (-1 = unconnected). Multiple sources can map to the same destination (many-to-one fan-in). Send connections follow the same one-source-to-one-destination model independently.

### numAllowedConnections Constraint

When `numAllowedConnections == 2` (the default for stereo processors), the matrix enforces auto-correction:

- Adding a third connection automatically removes the oldest even or odd connection
- Removing a connection that drops count below 2 auto-restores a default passthrough

After calling `setNumChannels(n)`, `numAllowedConnections` is also set to `n`, which relaxes the stereo constraint for multichannel configurations.

### clear() Behavior

`clear()` calls `resetToDefault()` (which sets stereo passthrough 0->0, 1->1) then explicitly removes those default connections. With the stereo constraint active (`numAllowedConnections == 2`), the auto-correction may re-add a default connection, so the matrix may not end up truly empty. See `clear()` for full details and the typical `clear()` then `addConnection()` rebuild pattern.

### Peak Metering

Peak values are only computed when the routing editor is actively displayed in the UI. See `getSourceGainValue()` for threading details and the editor-dependency pitfall.

### Query Method Return Types

`getSourceChannelsForDestination` and `getDestinationChannelForSource` accept either a single index or an array of indices, with polymorphic return types. See each method entry for return type details and the type-checking pitfall on `getSourceChannelsForDestination()`.

### Processors with Routing Matrices

| Base Class | Examples |
|-----------|----------|
| ModulatorSynth | All synth modules (SineSynth, WavetableSynth, ModulatorSynthChain, etc.) |
| VoiceEffectProcessor | Polyphonic effects |
| HardcodedProcessor | Hardcoded DSP modules |

Modulators, MIDI processors, and monophonic effects do NOT have routing matrices.

## obtainedVia
`Synth.getRoutingMatrix(processorId)` -- global search from MainSynthChain, or `ChildSynth.getRoutingMatrix()` from an existing synth handle.

## minimalObjectToken
rm

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| NumInputs | (dynamic) | int | Number of source channels at construction time | Channel Info |
| NumOutputs | (dynamic) | int | Number of destination channels at construction time | Channel Info |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `rm.setNumChannels(8);` on a non-resizeable matrix | Check processor type first | Most matrices have `resizeAllowed = false`. Only processors that explicitly allow resizing support `setNumChannels`. Throws "Can't resize this matrix". |
| Reading `rm.NumInputs` after `rm.setNumChannels(8)` expecting 8 | Call `rm.getNumSourceChannels()` instead | `NumInputs`/`NumOutputs` are snapshot constants set at construction time. They do not update when channel counts change. |

## codeExample
```javascript
// Get the routing matrix for a synth module
const var rm = Synth.getRoutingMatrix("SynthName");

// Route source channel 0 to destination channel 2
rm.addConnection(0, 2);

// Query current connections
Console.print(rm.getNumSourceChannels());
Console.print(rm.getDestinationChannelForSource(0));
```

## Alternatives
- `GlobalRoutingManager` -- cross-module signal routing via named cables and OSC, vs RoutingMatrix's intra-processor channel mapping.

## Related Preprocessors
`USE_BACKEND` (routing editor popup and Broadcaster MatrixViewer component).

## Diagrams

### routing-matrix-connection-model
- **Brief:** Channel Connection Arrays
- **Type:** topology
- **Description:** Shows two parallel fixed-size arrays (channelConnections and sendConnections), each with NUM_MAX_CHANNELS (16) slots. Each source index stores a single destination index or -1. Multiple sources can point to the same destination (fan-in). The ScriptRoutingMatrix wrapper accesses these arrays through RoutableProcessor::MatrixData, with SimpleReadWriteLock protecting concurrent access from audio and script threads.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods have clear runtime error reporting (script errors for invalid channels, non-resizeable matrices). No silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
