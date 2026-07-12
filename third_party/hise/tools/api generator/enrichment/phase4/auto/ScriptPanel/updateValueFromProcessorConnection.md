Reads the current value from the connected processor (set via the `processorId` and `parameterId` properties) and updates this component accordingly. Does nothing if no processor connection is established.

Special `parameterId` values:

| Index | Reads |
| --- | --- |
| `-2` | Modulation intensity from a Modulation processor |
| `-3` | Bypass state (`1.0` if bypassed, `0.0` if not) |
| `-4` | Inverted bypass state (`0.0` if bypassed, `1.0` if not) |
| `>= 0` | The attribute at the given parameter index |
