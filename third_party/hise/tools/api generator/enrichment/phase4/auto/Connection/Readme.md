# Connection

Connection is a read-only handle for inspecting and removing a wiring link between a source node and a target parameter in a scriptnode `DspNetwork` graph. Each Connection represents a single link from a modulation source or container macro parameter to a destination parameter.

![Scriptnode Class Hierarchy](topology_scriptnode-hierarchy.svg)

`Connection` represents a wiring link between two `Parameter` objects across different nodes in the same `DspNetwork`.

Connections are created through the source or target side of the link:

```js
// From the source node
const var cn = modNode.connectTo(targetParam, 0);

// From the target parameter
const var cn = param.addConnectionFrom(connectionData);
```

Once created, a Connection lets you:

- Query the source node and target parameter
- Check the modulation update rate (block size of the lowest common container)
- Test whether the connection is still active
- Remove the connection from the graph

> Connection objects become invalid after `disconnect()` is called or the connected node is deleted. After invalidation, `isConnected()` returns false and getter methods return undefined or zero. It is safe to hold a reference to a disconnected Connection - the object uses weak references internally.

## Common Mistakes

- **Wrong:** Using `cn.getSourceNode(false)` to find the signal generator
  **Right:** Use `cn.getSourceNode(true)` to trace through cable nodes
  *With `false`, the returned node may be a routing cable intermediary rather than the actual signal source. Pass `true` to trace through cable nodes to the real source.*
