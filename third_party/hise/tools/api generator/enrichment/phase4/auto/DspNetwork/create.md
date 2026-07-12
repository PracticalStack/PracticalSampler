Creates a node with the given factory path and ID, and returns a reference to it. The factory path uses `factory.node` format (e.g. `core.gain`, `container.split`, `math.add`). If `id` is empty, a unique name is auto-generated from the path suffix. Returns `undefined` if no registered factory matches the path.

```javascript
var gain = nw.create("core.gain", "myGain");
```

> [!Warning:Existing ID returns the existing node silently] If a node with the given `id` already exists, the method returns the existing node and ignores the `path` parameter. Calling `create("core.gain", "myOsc")` when a `core.oscillator` named "myOsc" already exists returns the oscillator, not a gain node, with no warning.
