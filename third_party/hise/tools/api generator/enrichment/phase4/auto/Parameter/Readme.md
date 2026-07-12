# Parameter

Parameter represents a single named parameter on a scriptnode node. Each parameter holds a double value within a configurable range and can receive modulation connections from other nodes or container macro parameters.

![Scriptnode Class Hierarchy](topology_scriptnode-hierarchy.svg)

`Parameter` belongs to a `Node` and can be wired to other parameters via `Connection` objects.

Use `setValue()` to change a parameter's value and `getValue()` to read it. See the `setValue()` and `setUseExternalConnection()` method pages for details on the two value-setting modes.

You can read and configure the parameter's range properties (minimum, maximum, skew, step size) and create or remove modulation connections from other nodes.

```js
const var p = nd.getOrCreateParameter("Volume");
```

The range system uses four constant identifiers defined on the Parameter object:

| Constant | Value | Description |
|----------|-------|-------------|
| `p.MinValue` | `"MinValue"` | Range minimum property ID |
| `p.MaxValue` | `"MaxValue"` | Range maximum property ID |
| `p.MidPoint` | `"MidPoint"` | Range skew midpoint property ID |
| `p.StepSize` | `"StepSize"` | Step interval property ID |

Use these constants with `setRangeProperty()` to modify individual range properties.

> [!Tip:Obtaining a Parameter reference] Parameter objects are obtained from a [Node]($API.Node$) via `Node.getOrCreateParameter()`. If the node is not yet fully connected in the network, value-setting calls are silently ignored.

## Common Mistakes

- **Remove existing connection before adding a new one**
  **Wrong:** `p.addConnectionFrom(connectionData)` on an already-automated parameter
  **Right:** Remove existing connection first with `p.addConnectionFrom(0)`, then add the new one
  *If the parameter's Automated flag is already true, `addConnectionFrom()` returns empty without creating a connection. No error is raised.*
