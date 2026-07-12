# Parameter -- Class Analysis

## Brief
Scriptnode node parameter with value, range properties, and modulation connection support.

## Purpose
Parameter represents a single named parameter on a scriptnode Node. It holds a double value within a configurable range (min, max, skew, step size) and can receive modulation connections from other nodes or container macro parameters. Parameter provides two value-setting paths -- async (immediate DSP update, no undo) and sync (ValueTree update with undo, deferred DSP update) -- to support both runtime automation and UI-driven interaction. Created and owned by Node instances, Parameter objects are the primary interface for programmatic control of node behavior within a DspNetwork graph.

## Details

### Value Setting: Async vs Sync

Parameter offers two distinct value-setting paths. See `setValueAsync()` and `setValueSync()` for full details.

| Aspect | setValueAsync | setValueSync |
|--------|---------------|--------------|
| DSP update | Immediate | Deferred (via ValueTree listener) |
| ValueTree update | Not performed | Immediate with undo |
| Undo support | No | Yes |
| Voice scoping | All voices (NoVoiceSetter) | Via the async path |
| Use case | Runtime automation, modulation | UI interaction, preset recall |

`setValueSync()` stores the value to the ValueTree, which triggers an internal listener that calls `setValueAsync()` -- so both paths eventually reach the DSP callback.

### Range System

Parameters use the scriptnode range property set:

| Property | Default | Description |
|----------|---------|-------------|
| MinValue | 0.0 | Range minimum |
| MaxValue | 1.0 | Range maximum |
| SkewFactor | 1.0 | Logarithmic skew |
| StepSize | 0.0 | Discrete step interval (0 = continuous) |
| Inverted | false | Whether the range mapping is inverted |

See `getRangeObject()` for reading the full range, `setRangeFromObject()` for bulk updates (with undo), and `setRangeProperty()` for individual property changes (without undo).

### Connection System

See `addConnectionFrom()` for the full connection API. It operates in two modes based on argument type: object argument to create a connection, non-object argument to remove it.

### Dynamic Parameter Callback

Internally, each Parameter holds a `parameter::dynamic_base` pointer -- the actual DSP callback function. This is set when the node is initialized or when connections change. Value-setting methods call through this pointer. If the dynamic parameter is null (node not yet initialized), `setValueAsync()` silently does nothing.

## obtainedVia
`Node.getOrCreateParameter(indexOrId)` -- returns an existing parameter by index or name, or creates a new one on container nodes.

## minimalObjectToken
p

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| MinValue | "MinValue" | String | Range minimum property ID | Range |
| MaxValue | "MaxValue" | String | Range maximum property ID | Range |
| MidPoint | "MidPoint" | String | Range skew midpoint property ID | Range |
| StepSize | "StepSize" | String | Step interval property ID | Range |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `p.addConnectionFrom(connectionData)` on an already-automated parameter | Remove existing connection first with `p.addConnectionFrom(0)`, then add new one | If the parameter's Automated flag is already true, `addConnectionFrom()` returns empty without creating a connection. |

## codeExample
```javascript
// Get a parameter from a node
const var p = nd.getOrCreateParameter("Volume");

// Read current value and range
var val = p.getValue();
var range = p.getRangeObject();

// Set value (immediate DSP update, no undo)
p.setValueAsync(0.5);

// Set value (with undo, deferred DSP update)
p.setValueSync(0.75);

// Modify range
p.setRangeProperty(p.MinValue, 0.0);
p.setRangeProperty(p.MaxValue, 100.0);
```

## Alternatives
- `Node` -- the processing unit that owns one or more Parameters; Parameter controls a single value on a Node.
- `Connection` -- the link object that routes modulation from a source node to a Parameter; Parameter is the target endpoint that holds the value.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- Parameter.setRangeProperty -- value-check (logged)
