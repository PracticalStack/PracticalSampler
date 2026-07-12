Defines the custom automation slot layout. Pass an array of JSON objects, each describing one automation slot with its ID, value range, display format, and connection targets. Each slot becomes a DAW-visible plugin parameter (if `allowHostAutomation` is true) and can be mapped to MIDI CC (if `allowMidiAutomation` is true).

Each slot definition supports these properties:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ID` | String | (required) | Unique identifier for the slot |
| `min` | Number | 0.0 | Range minimum |
| `max` | Number | 1.0 | Range maximum |
| `middlePosition` | Number | - | Applies skew so this value appears at the centre of the range |
| `stepSize` | Number | 0.0 | Quantisation step (0 = continuous) |
| `defaultValue` | Number | range min | Initial value, clamped to range |
| `allowMidiAutomation` | Boolean | true | Whether MIDI CC can control this slot |
| `allowHostAutomation` | Boolean | true | Whether the DAW host can control this slot |
| `pluginParameterGroup` | String | "" | Group name (must be registered via `setPluginParameterGroupNames` first) |
| `connections` | Array | (required) | Connection target objects (can be empty) |
| `mode` | String | - | Value-to-text display mode (see below) |
| `options` | Array | - | Discrete option labels (alternative to `mode`) |
| `suffix` | String | "" | Suffix for numeric display |

Connection targets use one of three patterns:

- `{"processorId": "...", "parameterId": "..."}` - directly sets a module parameter
- `{"automationId": "..."}` - routes to another automation slot (meta-parameter)
- `{"cableId": "..."}` - bidirectional link to a global routing cable

The `mode` property controls how the DAW displays the parameter value:

| Mode | Display |
|------|---------|
| `"Frequency"` | Hz/kHz |
| `"Time"` | ms/s |
| `"TempoSync"` | Tempo names (1/4, 1/8T) |
| `"Pan"` | L/C/R |
| `"NormalizedPercentage"` | 0-100% |
| `"Decibel"` | dB |
| `"Semitones"` | st |

> [!Warning:MetaConnection targets must appear first] MetaConnection targets (referenced via `automationId`) must appear earlier in the automation array than the slot that references them. Forward references to later slots are not resolved.
