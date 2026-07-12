Recursively creates a tree of nodes from a JSON descriptor and adds them to the given parent container. Each object in the descriptor must have `FactoryPath` and `ID` properties. An optional `Nodes` array defines child nodes, processed recursively. Returns the top-level node, or `undefined` if creation fails.

```javascript
var tree = {
    "FactoryPath": "container.chain",
    "ID": "subChain",
    "Nodes": [
        { "FactoryPath": "core.gain", "ID": "gain1" },
        { "FactoryPath": "core.gain", "ID": "gain2" }
    ]
};

var result = nw.createFromJSON(tree, rootNode);
```
