Returns the value of a node-type-specific property. Pass the property ID as a string - each node type registers its own property names (e.g. "Mode", "Frequency") which are also available as constants on the node instance (e.g. `node.Mode`).

> [!Warning:Only reads node-type-specific properties] Does not return direct node attributes such as NodeColour, Comment, or Folded, even though `set()` can write to them. Use `isBypassed()` to read the bypass state.
