# updateValueFromProcessorConnection | UNSAFE

Reads the current attribute value from the connected processor (set via the `processorId` and `parameterId` properties) and calls `setValue()` with that value. Does nothing if no processor connection is established.

```
updateValueFromProcessorConnection()
```

## Special parameterId Values

| Index | Reads |
|-------|-------|
| `-2` | Modulation intensity from a Modulation processor |
| `-3` | Bypass state (1.0 if bypassed, 0.0 if not) |
| `-4` | Inverted bypass state (0.0 if bypassed, 1.0 if not) |
| `>= 0` | Attribute at the given parameter index |

## Required Setup

The `processorId` and `parameterId` properties must be set on this component to establish the processor connection.

## Pair With

- `setValue()` - called internally with the read value

## Source

`ScriptingApiContent.h` line ~1734
