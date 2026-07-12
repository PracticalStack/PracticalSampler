<!-- Diagram triage:
  - routing-matrix-connection-model: CUT (connection model maps to compact table; two arrays with source-to-dest mapping is clear from prose without a visual)
-->

# RoutingMatrix

RoutingMatrix controls the internal audio channel routing of a single processor, mapping source channels to destination channels. Obtain it from any processor that supports routing:

```js
const var rm = Synth.getRoutingMatrix("ProcessorName");
```

The matrix maintains two independent connection layers:

| Layer | Purpose | Add | Remove |
|-------|---------|-----|--------|
| Primary | Main source-to-destination routing | `addConnection()` | `removeConnection()` |
| Send | Parallel send bus | `addSendConnection()` | `removeSendConnection()` |

Each source channel maps to at most one destination in each layer. Multiple sources can map to the same destination (fan-in). By default, a new matrix has stereo passthrough: source 0 to destination 0, source 1 to destination 1.

For multi-output plugins that route audio to more than one stereo output pair, call `setNumChannels()` before adding connections beyond channels 0 and 1. This expands the source channel count and relaxes the default stereo constraint that would otherwise auto-correct your routing. You can also configure routing visually in the HISE IDE by clicking a processor's channel meter to open the routing editor.

The class exposes two constants, `RoutingMatrix.NumInputs` and `RoutingMatrix.NumOutputs`, but these are snapshot values captured at construction time. Use `getNumSourceChannels()` and `getNumDestinationChannels()` to read the live channel counts after resizing.

> [!Tip:Multi-output needs custom channel build] Multi-output routing requires a custom HISE build with increased channel limits. Set `NUM_MAX_CHANNELS` and `HISE_NUM_PLUGIN_CHANNELS` to the required channel count (must be a multiple of 2) in the Projucer preprocessor definitions, then rebuild HISE. Add the same definitions to your project settings when exporting the plugin.

> [!Tip:Only synths and polyphonic effects have matrices] Only synth modules, polyphonic effects, and hardcoded DSP modules have routing matrices. Modulators, MIDI processors, and monophonic effects do not.

## Common Mistakes

- **Use getNumSourceChannels not NumInputs**
  **Wrong:** Reading `rm.NumInputs` after calling `setNumChannels(8)` and expecting 8.
  **Right:** Call `rm.getNumSourceChannels()` instead.
  *`NumInputs` and `NumOutputs` are snapshot constants from construction time. They do not update when the channel count changes.*

- **Call setNumChannels before multichannel routing**
  **Wrong:** Calling `addConnection(0, 4)` without first calling `setNumChannels()`.
  **Right:** Call `setNumChannels(6)` or higher before routing beyond the default stereo pair.
  *The default stereo constraint auto-removes connections when you add a third. `setNumChannels` relaxes this constraint.*

- **Clear matrix before rebuilding routing**
  **Wrong:** Adding new connections without clearing the matrix first when rebuilding a routing configuration.
  **Right:** Call `clear()` then add all connections fresh.
  *Old connections from a previous configuration persist and create unexpected signal paths.*

- **Check addConnection return value**
  **Wrong:** Not checking the return value of `addConnection()` when routing to higher channel pairs.
  **Right:** `local ok = rm.addConnection(1, 5); if(!ok) { /* fallback */ }`
  *The host DAW may not support the requested output channels. The method returns `false` when the destination is unavailable.*

**See also:** $SN.routing.matrix$ -- Scriptnode node for channel routing matrix in DspNetwork
