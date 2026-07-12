Configures the pre-callback to receive a JSON representation of the preset data instead of a file reference. When enabled, the JSON object passed to `setPreCallback` can be modified in-place before loading - enabling version migration, value remapping, and conditional data transformation.

The preset data object has this structure:

```json
{
  "version": "1.0.0",
  "Content": [ ... ],        // array of component value objects ({id, value, ...})
  "Modules": { ... },        // module state data
  "MidiAutomation": { ... }, // MIDI automation mappings
  "MPEData": { ... }         // MPE configuration
}
```

When the second parameter (`shouldUnpackComplexData`) is true, JSON-encoded strings and Base64-encoded data within the preset are decoded back to native objects, making the data fully inspectable. Only enable this when you need to inspect encoded content (e.g. migrating sample map references) - it adds conversion overhead to every preset load.

> [!Warning:Requires registered pre-callback] Preprocessing has no effect without a pre-callback registered via `setPreCallback`. Enabling it alone just adds unnecessary conversion overhead on every load.
