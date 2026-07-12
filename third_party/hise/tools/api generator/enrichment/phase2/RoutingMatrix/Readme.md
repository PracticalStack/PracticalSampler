# RoutingMatrix -- Project Context

## Project Context

### Real-World Use Cases
- **Multi-output plugin routing**: A multichannel instrument (e.g., a drum machine with 12+ channels) uses RoutingMatrix to route each channel strip's stereo output to a user-selectable DAW output bus pair. The master synth chain matrix is expanded to 16+ channels, and per-channel container matrices are dynamically remapped when the user changes output assignments via ComboBox selectors. Broadcasters with `attachToRoutingMatrix` keep the UI synchronized.
- **Builder API channel strip construction**: When programmatically constructing a module tree with the Builder API, RoutingMatrix configures internal multichannel audio routing for each processor -- expanding channel counts with `setNumChannels`, isolating effect processors to specific channel pairs with `clear()` + `addConnection()`, and wiring mixer/send buses.
- **M/S processing chain**: An effect chain uses RoutingMatrix to isolate mid and side signals by selectively removing one channel of a stereo pair from consecutive EQ processors, creating a mid-only and side-only processing path between LR-to-MS and MS-to-LR converter effects.
- **Basic multichannel output selection**: A simpler plugin uses RoutingMatrix to let the user select which stereo output pair receives audio, checking the `addConnection` return value to detect whether the host supports the requested channel count and falling back to stereo if not.

### Complexity Tiers
1. **Stereo output selection** (simplest): `Synth.getRoutingMatrix()`, `addConnection()` with return value checking. Sufficient for plugins that route a stereo signal to one of several output bus pairs.
2. **Multi-output with dynamic remapping**: `setNumChannels()`, `clear()`, loop-based `addConnection()` rebuild, Broadcaster `attachToRoutingMatrix` for UI sync. Used when each channel needs independent output bus assignment.
3. **Builder API multichannel construction** (advanced): `builder.get(processor, builder.InterfaceTypes.RoutingMatrix)`, `setNumChannels()`, `clear()` + targeted `addConnection()` per effect, `removeConnection()` for channel isolation. Used when programmatically building complex signal flow topologies.

### Practical Defaults
- Use `clear()` followed by `addConnection()` calls when rebuilding a routing configuration. This is more reliable than trying to selectively remove and add individual connections.
- Call `setNumChannels()` before any multichannel `addConnection()` calls. It both expands the source channel count and relaxes the stereo constraint that would otherwise auto-correct your connections.
- Use `Synth.getRoutingMatrix("ProcessorName")` for runtime routing changes and `builder.get(proc, builder.InterfaceTypes.RoutingMatrix)` during Builder API construction. Both return the same RoutingMatrix object type.
- Check the return value of `addConnection()` when routing to higher output pairs (3/4, 5/6, etc.) -- the host may not support that many channels, and the method returns `false` on failure.

### Integration Patterns
- `Synth.getRoutingMatrix()` -> `RoutingMatrix.setNumChannels()` -> `RoutingMatrix.addConnection()` -- standard multichannel setup sequence.
- `Builder.get(proc, InterfaceTypes.RoutingMatrix)` -> `RoutingMatrix.clear()` -> `RoutingMatrix.addConnection()` -- Builder API routing configuration for each created processor.
- `Broadcaster.attachToRoutingMatrix(moduleIds, metadata)` -> listener callback `(processorId, matrix)` -- reactive UI updates when routing changes, used alongside ComboBox selectors for output bus assignment.
- `RoutingMatrix.clear()` + `RoutingMatrix.addConnection(0, 0)` / `addConnection(1, 1)` -- stereo passthrough reset pattern when disabling multi-output mode.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `addConnection(0, 4)` without first calling `setNumChannels()` | Call `setNumChannels(6)` or higher first | The default stereo constraint (`numAllowedConnections == 2`) auto-removes connections when you add a third. `setNumChannels` relaxes this constraint. |
| Not checking the return value of `addConnection()` for higher channel pairs | `local ok = rm.addConnection(1, 5); if(!ok) { /* fallback */ }` | The host DAW may not support the requested output channels. The method returns `false` when the destination channel is unavailable. |
| Rebuilding routing by adding connections without clearing first | Call `clear()` then add all connections fresh | Without clearing, old connections from a previous configuration may persist and create unexpected signal routing. |
