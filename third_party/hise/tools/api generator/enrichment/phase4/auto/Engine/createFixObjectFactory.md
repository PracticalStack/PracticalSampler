Creates a factory for fixed-layout objects with typed fields. Pass a JSON object where property names become field names and values define the type: integers for Integer fields, floats for Float fields, booleans for Boolean fields, or arrays of those for array-typed fields. These objects are more performant than regular JSON objects for large collections because they use contiguous memory with known offsets.

```js
var factory = Engine.createFixObjectFactory({
    "x": 0, "y": 0.0, "active": false
});
```