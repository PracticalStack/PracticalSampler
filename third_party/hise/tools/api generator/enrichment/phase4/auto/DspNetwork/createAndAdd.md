Creates a node and immediately adds it as the last child of the given parent container. Equivalent to calling `create(path, id)` followed by `node.setParent(parent, -1)`. If `id` is empty, a unique name is auto-generated from the path suffix.

```javascript
var node = nw.createAndAdd("core.gain", "myGain", parentContainer);
```
