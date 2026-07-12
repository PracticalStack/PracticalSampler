# Connection -- Class Analysis

## Brief
Scriptnode connection handle linking a source node or macro parameter to a target parameter.

## Purpose
Connection represents the wiring between a source (container macro parameter or modulation source node) and a target parameter within a scriptnode DspNetwork graph. It is a read-only introspection handle -- once created via `Node.connectTo()` or `Parameter.addConnectionFrom()`, Connection lets you query the source node, target parameter, update rate, and validity, or remove the connection entirely. Connection objects wrap a ValueTree entry in the network's connection tree and become invalid once the connection is removed.

## Details

### Connection Sources

Connections originate from two kinds of source nodes:

| Source Type | How Created | ValueTree Location |
|-------------|------------|--------------------|
| Container macro parameter | `Node.connectTo()` on a container node, or `Parameter.addConnectionFrom()` with a container's macro info | `Parameter/Connections` child tree on the source macro parameter |
| Modulation source node | `Node.connectTo()` on a modulation node, or `Parameter.addConnectionFrom()` with modulation source info | `ModulationTargets` child tree on the source node |
| Multi-output modulation | `Node.connectTo()` on a node with SwitchTargets | `SwitchTargets/SwitchTarget[n]/Connections` child tree |

The connection's source is implicit (determined by where the ValueTree lives in the hierarchy), while the target is explicit (stored as `NodeId` and `ParameterId` properties on the connection ValueTree).

### Signal Source Tracing

See `getSourceNode()` for details on tracing through cable intermediaries vs. returning the immediate source node.

### Update Rate

See `getUpdateRate()` for details on block-size-based update rate resolution between source and target nodes.

### Object Lifetime

Connection objects become invalid when the underlying ValueTree is removed from its parent (either via `disconnect()` or when a connected node is deleted). After invalidation, `isConnected()` returns false and getter methods return empty/null/zero values. The class uses WeakReferences internally, so it is safe against node deletion. See `disconnect()` and `isConnected()` for details.

## obtainedVia
`Node.connectTo(parameterTarget, sourceInfo)` or `Parameter.addConnectionFrom(connectionData)` -- connections are created through the source node or target parameter.

## minimalObjectToken
cn

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using `cn.getSourceNode(false)` to find who generates the signal | Use `cn.getSourceNode(true)` to trace through cable nodes | With `false`, you get the immediate source which may be a routing cable node, not the actual signal generator. Pass `true` to trace through intermediaries. |

## codeExample
```javascript
// Connect a modulation source to a target parameter
const var cn = modNode.connectTo(targetParam, 0);

// Inspect the connection
Console.print(cn.isConnected());           // true
Console.print(cn.getUpdateRate());         // block size (e.g. 512)

var source = cn.getSourceNode(true);       // actual signal source
var target = cn.getTarget();               // target Parameter object

// Remove the connection
cn.disconnect();
Console.print(cn.isConnected());           // false
```

## Alternatives
- `Parameter` -- the target endpoint that receives the value; Connection is the link object that routes a signal to a Parameter.
- `Node` -- the processing unit that produces or consumes the signal; Connection represents the wiring between nodes.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Connection methods are simple ValueTree/WeakReference reads with no silent-failure preconditions. The only mutating method (disconnect) has immediate observable effect. No parse-time diagnostics warranted.
