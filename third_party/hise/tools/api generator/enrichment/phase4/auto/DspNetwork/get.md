Returns a reference to the node with the given ID. If the ID matches the network's own ID, returns the root node. Returns `undefined` if no node with the given ID exists. Also accepts a Node reference, which is returned as-is.

```javascript
var node = nw.get("myGain");
var root = nw.get(nw.getId());
```
