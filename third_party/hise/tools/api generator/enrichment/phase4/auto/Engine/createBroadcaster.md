Creates a broadcaster for sending messages to attached listeners. Pass a JSON object with `id` and `args` properties to define the broadcaster's identity and argument schema. The `args` property can be an array of argument name strings or a JSON object mapping names to default values. The argument count determines which `attach*` source methods are compatible.

```js
const var bc = Engine.createBroadcaster({
    "id": "MyBroadcaster",
    "args": ["value", "source"]
});
```