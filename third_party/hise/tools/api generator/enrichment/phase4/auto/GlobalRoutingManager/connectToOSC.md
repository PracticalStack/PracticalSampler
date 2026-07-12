Establishes a bidirectional OSC connection for external controller communication. Pass a JSON configuration object and an error callback (or `false` to clear a previous error handler). Calling again with different settings tears down the previous connection first.

The configuration object accepts the following properties:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Domain` | String | `"/hise_osc_receiver"` | Root OSC address prefix. Must start with `/` and must not end with `/`. |
| `SourceURL` | String | `"127.0.0.1"` | Receiver bind address. |
| `SourcePort` | Integer | `9000` | Port for listening to incoming OSC messages. |
| `TargetURL` | String | `"127.0.0.1"` | Sender target address. |
| `TargetPort` | Integer | `-1` | Port for outgoing OSC messages. Set to `-1` or omit to create a receive-only connection. |
| `Parameters` | JSON | `{}` | Map of cable sub-addresses to range objects for bidirectional value normalisation. Each entry uses the cable's sub-address as key and a range object (`MinValue`, `MaxValue`, `SkewFactor`, `StepSize`) as value. |

When a `TargetPort` is specified, cables whose IDs start with `/` automatically send their value changes as outgoing OSC messages. The `Parameters` map transforms incoming and outgoing cable values through the defined range - useful when the external OSC source operates outside the 0.0 to 1.0 range.

> [!Tip:Use the OSCLogger FloatingTile for debugging] The OSCLogger FloatingTile logs all incoming OSC messages with filtering and cable-based colour coding. Add it to your interface during development to verify that messages arrive with the expected addresses and values.
