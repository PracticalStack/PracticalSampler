Creates a connection from this node's output to a target parameter. The behaviour depends on the node type:

- **Container nodes:** Creates a macro parameter connection. sourceInfo is the name (string) of the source macro parameter on this container.
- **Modulation source nodes:** Creates a modulation target connection. sourceInfo is the output slot index (integer). Nodes with multiple modulation outputs (e.g. `control.xfader`) use different slot indices for each output.

Single-output modulation source (`control.peak` has one output at slot 0):

```javascript
const var peak = nw.create("control.peak", "myPeak");
const var gainParam = gainNode.getOrCreateParameter("Gain");
peak.connectTo(gainParam, 0);
```

Multi-output modulation source (`control.xfader` has two outputs, one per crossfade side):

```javascript
const var xfader = nw.create("control.xfader", "myXF");
const var paramA = nodeA.getOrCreateParameter("Gain");
const var paramB = nodeB.getOrCreateParameter("Gain");

// Slot 0: first crossfade output
xfader.connectTo(paramA, 0);

// Slot 1: second crossfade output
xfader.connectTo(paramB, 1);
```

To connect a container parameter to another parameter in the opposite direction, use [Parameter.addConnectionFrom()]($API.Parameter.addConnectionFrom$) instead.

> [!Warning:Silent failure on unsupported node types] On leaf nodes that are not modulation sources, this method silently returns undefined without creating a connection or reporting an error.
